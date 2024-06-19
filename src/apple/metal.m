#include "foundation/udata.h"
#include "gpu/gpu.h"
#include "foundation/logger.h"
#include "gpu/gpu_const.h"

// see https://clang.llvm.org/docs/LanguageExtensions.html#automatic-reference-counting
#include <Foundation/Foundation.h>
#include <TargetConditionals.h>
#include <AvailabilityMacros.h>
#import <Metal/Metal.h>
#include <objc/objc.h>
#include <assert.h>
#include <stdlib.h>
#import <QuartzCore/CoreAnimation.h> // needed for CAMetalDrawable
#import "metal.h"

typedef struct gpu_shader_mtl_t {
    id<MTLLibrary> vertex_lib;
    id<MTLLibrary> fragment_lib;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;
} gpu_shader_mtl_t;

typedef struct gpu_texture_mtl_t {
    id<MTLTexture> texture;
    NSUInteger width, height, depth;
    gpu_pixel_format format;
    gpu_texture_type type;
} gpu_texture_mtl_t;

typedef struct gpu_buffer_mtl_t {
    id<MTLBuffer> buffer;
} gpu_buffer_mtl_t;

typedef struct gpu_pipeline_mtl_t {
    id<MTLRenderPipelineState> pso;
    id<MTLDepthStencilState> dso;
    bool instanced, indexed;

    MTLPrimitiveType primitive_type;
    MTLIndexType index_type;
    MTLCullMode cull_mode;
    MTLWinding winding;
    MTLPixelFormat depth_stencil_format;
    u32 stencil_ref;
} gpu_pipeline_mtl_t;

typedef struct gpu_pass_mtl_t {
    int width, height;
} gpu_pass_mtl_t;

typedef struct gpu_swapchain_mtl_t {
    int width, height, sample_count;
    gpu_pixel_format color_format;
    gpu_pixel_format depth_stencil_format;
    id<CAMetalDrawable> drawable;
    id<MTLTexture> color_texture;
    id<MTLTexture> depth_stencil_texture;
} gpu_swapchain_mtl_t;

typedef struct gpu_device_mtl_t {
    dispatch_semaphore_t semaphore;
    int frame_index, frame_swap_count;
    gpu_swapchain_mtl_t swapchain;

    id<MTLDevice> device;
    id<MTLHeap> heap;
    id<MTLCommandQueue> cmd_queue;
    id<MTLCommandBuffer> cmd_buffer;
    id<MTLRenderCommandEncoder> cmd_encoder;
    id<CAMetalDrawable> cur_drawable;
    id<MTLBuffer> uniform_buffers[GPU_SWAP_BUFFER_COUNT];

    gpu_shader_mtl_t shaders[GPU_SHADER_POOL_SIZE];
    gpu_texture_mtl_t textures[GPU_TEXTURE_POOL_SIZE];
    gpu_buffer_mtl_t buffers[GPU_BUFFER_POOL_SIZE];
    gpu_pipeline_mtl_t pipelines[GPU_PIPELINE_POOL_SIZE];
} gpu_device_mtl_t;

typedef struct gpu_state_mtl_t {
    bool valid;
    int frame_index;
    gpu_device_mtl_t device;

    gpu_pipeline_mtl_t cur_pipeline;
    gpu_pass_mtl_t cur_pass;
    gpu_buffer_mtl_t cur_index_buffer;
} gpu_state_mtl_t;

static gpu_state_mtl_t _state;

#define _mtl_invalid_id 0
static int _mtl_shader_id = 1;
static int _mtl_texture_id = 1;
static int _mtl_buffer_id = 1;
static int _mtl_pipeline_id = 1;

int _mtl_add_texture(gpu_texture_mtl_t texture) {
    int id = _mtl_texture_id++;
    _state.device.textures[id] = texture;
    return id;
}

gpu_texture_mtl_t _mtl_get_texture(int id) { return _state.device.textures[id]; }

int _mtl_add_buffer(gpu_buffer_mtl_t buffer) {
    int id = _mtl_buffer_id++;
    _state.device.buffers[id] = buffer;
    return id;
}

gpu_buffer_mtl_t _mtl_get_buffer(int id) { return _state.device.buffers[id]; }

int _mtl_add_shader(gpu_shader_mtl_t shader) {
    int id = _mtl_shader_id++;
    _state.device.shaders[id] = shader;
    return id;
}

gpu_shader_mtl_t _mtl_get_shader(int id) { return _state.device.shaders[id]; }

int _mtl_add_pipeline(gpu_pipeline_mtl_t pipeline) {
    int id = _mtl_pipeline_id++;
    _state.device.pipelines[id] = pipeline;
    return id;
}

gpu_pipeline_mtl_t _mtl_get_pipeline(int id) { return _state.device.pipelines[id]; }

bool gpu_request_device(os_window_t *window) {
    _state.device.semaphore = dispatch_semaphore_create(GPU_SWAP_BUFFER_COUNT);
    _state.device.device = MTLCreateSystemDefaultDevice();

    if (nil == _state.device.device) {
        _state.valid = false;
        return false;
    }

    uint slot_count = GPU_SWAP_BUFFER_COUNT * (
        GPU_BUFFER_POOL_SIZE + 
        GPU_TEXTURE_POOL_SIZE + 
        GPU_PIPELINE_POOL_SIZE +
        GPU_SHADER_POOL_SIZE + 
        GPU_ATTACHMENTS_POOL_SIZE
    );

    _state.device.heap = nil;
    _state.device.cmd_queue = [_state.device.device newCommandQueue];
    _state.device.cmd_buffer = nil;
    _state.device.cmd_encoder = nil;
    _state.device.cur_drawable = nil;
    _state.device.frame_index = 0;
    _state.device.frame_swap_count = GPU_SWAP_BUFFER_COUNT;
    for (int i = 0; i < GPU_SWAP_BUFFER_COUNT; i++) {
        _state.device.uniform_buffers[i] = nil;
    }
    return true;
}

void gpu_destroy_device() {
    for (int i = 0; i < GPU_SWAP_BUFFER_COUNT; i++) {
        if (_state.device.uniform_buffers[i]) {
            [_state.device.uniform_buffers[i] release];
        }
    }
    [_state.device.cmd_encoder release];
    [_state.device.cmd_buffer release];
    [_state.device.cmd_queue release];
    [_state.device.device release];
    dispatch_release(_state.device.semaphore);
}

gpu_texture gpu_create_texture(gpu_texture_desc *desc) {
    assert(desc->width > 0);
    assert(desc->height > 0);
    MTLTextureDescriptor *mtl_desc = [MTLTextureDescriptor new];
    mtl_desc.textureType = _mtl_texture_type(desc->type);
    mtl_desc.pixelFormat = _mtl_pixel_format(desc->format);
    mtl_desc.width = (NSUInteger)desc->width;
    mtl_desc.height = (NSUInteger)desc->height;
    if (desc->type == TEXTURE_3D)
        mtl_desc.depth = (NSUInteger)desc->depth;
    else
        mtl_desc.arrayLength = 1;
    mtl_desc.usage = MTLTextureUsageShaderRead;

    id<MTLTexture> texture = [_state.device.device newTextureWithDescriptor: mtl_desc];
    gpu_texture_mtl_t mtl_texture = (gpu_texture_mtl_t){
        .texture = texture,
        .width = mtl_desc.width,
        .height = mtl_desc.height,
        .depth = mtl_desc.depth,
        .format = desc->format,
        .type = desc->type
    };
    gpu_texture result = { .id = _mtl_add_texture(mtl_texture) };
    
    if (desc->data.length > 0) {
        gpu_update_texture(result, desc->data);
    }

    return result;
}

void gpu_update_texture(gpu_texture texture, udata data) {
    gpu_texture_mtl_t mtl_texture = _mtl_get_texture(texture.id);
    int width = (int)mtl_texture.width;
    int height = (int)mtl_texture.height;
    gpu_pixel_format format = mtl_texture.format;

    int bytes_per_row = 0;
    int bytes_per_slice = 0;
    if (!_mtl_pixel_format_is_pvrtc(format)) {
        bytes_per_row = gpu_pixel_format_row_pitch(format, width, 1);
        bytes_per_slice = gpu_pixel_format_surface_pitch(format, width, height, 1);
    }

    MTLRegion region;
    int bytes_per_image;
    if (mtl_texture.type == TEXTURE_3D) {
        region = MTLRegionMake3D(0, 0, 0, width, height, mtl_texture.depth);
        bytes_per_image = bytes_per_slice;
    } else {
        region = MTLRegionMake2D(0, 0, width, height);
        bytes_per_image = 0;
    }

    [mtl_texture.texture replaceRegion: MTLRegionMake2D(0, 0, mtl_texture.width, mtl_texture.height)
        mipmapLevel: 0
        withBytes: data.data
        bytesPerRow: (NSUInteger)bytes_per_row];
}

gpu_buffer gpu_create_buffer(gpu_buffer_desc *desc) {
    assert(desc->size > 0);
    id<MTLBuffer> buffer = [_state.device.device newBufferWithLength: desc->size options: MTLResourceStorageModeManaged];
    gpu_buffer_mtl_t mtl_buffer = (gpu_buffer_mtl_t){ .buffer = buffer };
    return (gpu_buffer){ .id = _mtl_add_buffer(mtl_buffer) };
}

void gpu_update_buffer(gpu_buffer buffer, udata data) {
    gpu_buffer_mtl_t mtl_buffer = _mtl_get_buffer(buffer.id);
    memcpy([mtl_buffer.buffer contents], data.data, data.length);
#if defined(OS_MACOS)
    [mtl_buffer.buffer didModifyRange: NSMakeRange(0, data.length)];
#endif
}

gpu_binding gpu_create_binding(gpu_binding_desc *desc) {
    // gpu_pipeline_mtl_t mtl_pipeline = _mtl_get_pipeline(pipeline.id);
    // if (mtl_pipeline.instanced) {
    //     for (int i = 0; i < GPU_SWAP_BUFFER_COUNT; ++i) {
    //         if (nil == _state.device.uniform_buffers[i]) {
    //             _state.device.uniform_buffers[i] = [_state.device.device newBufferWithLength: GPU_UNIFORM_BUFFER_SIZE options: MTLResourceStorageModeManaged];
    //         }
    //         [binding->buffers[GPU_VERTEX_BUFFER_COUNT + i] release];
    //         binding->buffers[GPU_VERTEX_BUFFER_COUNT + i] = _state.device.uniform_buffers[i];
    //     }
    // }
    return (gpu_binding){ .id = 0};
}

gpu_mesh gpu_create_mesh(gpu_mesh_desc *desc) {
    // gpu_buffer vertex_buffer = gpu_create_buffer(&desc->vertex_buffer);
    // gpu_buffer index_buffer = gpu_create_buffer(&desc->index_buffer);
    // gpu_texture textures[GPU_SHADER_TEXTURE_COUNT];
    // for (int i = 0; i < GPU_SHADER_TEXTURE_COUNT; ++i) {
    //     if (desc->textures[i].data.length > 0) {
    //         textures[i] = gpu_create_texture(&desc->textures[i]);
    //     }
    // }
    // gpu_shader shader = gpu_create_shader(&desc->shader);
    // gpu_pipeline pipeline = gpu_create_pipeline(&desc->pipeline);
    // gpu_binding binding = gpu_create_binding(&desc->binding);
    return (gpu_mesh){ .id = 0};
}

void gpu_mtl_begin_frame(MTKView *view) {
    _state.device.swapchain = (gpu_swapchain_mtl_t) {
        .width = (int) [view drawableSize].width,
        .height = (int) [view drawableSize].height,
        .sample_count = (int) [view sampleCount],
        .color_format = PIXELFORMAT_BGRA8,
        .depth_stencil_format = PIXELFORMAT_NONE,
        .drawable = [view currentDrawable],
        .color_texture = [view multisampleColorTexture],
        .depth_stencil_texture = [view depthStencilTexture],
    };
}

void gpu_begin_render_pass(gpu_render_pass pass) {
    assert(_state.device.cmd_encoder == nil);
    assert(_state.device.cur_drawable == nil);

    // gpu_pass_action action = pass->action;
    // if (nil == _state.device.cmd_buffer) {
    //     dispatch_semaphore_wait(_state.device.semaphore, DISPATCH_TIME_FOREVER);
    //     _state.device.cmd_buffer = [_state.device.cmd_queue commandBuffer];
    //     [_state.device.cmd_buffer enqueue];
    //     [_state.device.cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> cmd_buf) {
    //         dispatch_semaphore_signal(_state.device.semaphore);
    //     }];
    // }
    
    // MTLRenderPassDescriptor *pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
    // gpu_attachments attachments = pass->attachments;
    // if (attachments.id == _mtl_invalid_id) {
    //     _state.device.cur_drawable = _state.device.swapchain.drawable;
    //     pass_desc.colorAttachments[0].texture = _state.device.cur_drawable.texture;
    //     pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    //     pass_desc.colorAttachments[0].loadAction = _mtl_load_action(action.color_action[0].load_action);
    //     gpu_color c = action.color_action[0].clear_value;
    //     pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(c.r, c.g, c.b, c.a);
    
    //     if (_state.device.swapchain.depth_stencil_texture) {
    //         pass_desc.depthAttachment.texture = _state.device.swapchain.depth_stencil_texture;
    //         pass_desc.depthAttachment.storeAction = MTLStoreActionStore;
    //         pass_desc.depthAttachment.loadAction = _mtl_load_action(action.depth_action.load_action);
    //         pass_desc.depthAttachment.clearDepth = action.depth_action.clear_value;
    //         if (_mtl_stencil_enabled_format(_state.device.swapchain.depth_stencil_format)) {
    //             pass_desc.stencilAttachment.texture = _state.device.swapchain.depth_stencil_texture;
    //             pass_desc.stencilAttachment.storeAction = MTLStoreActionStore;
    //             pass_desc.stencilAttachment.loadAction = _mtl_load_action(action.stencil_action.load_action);
    //             pass_desc.stencilAttachment.clearStencil = action.stencil_action.clear_value;
    //         }
    //     }
    // }

    // _state.device.cmd_encoder = [_state.device.cmd_buffer renderCommandEncoderWithDescriptor: pass_desc];
    // if (nil == _state.device.cmd_encoder) {
    //     return false;
    // }
    // return true;
}

void gpu_end_pass() {
    if (nil != _state.device.cmd_encoder) {
        [_state.device.cmd_encoder endEncoding];
        _state.device.cmd_encoder = nil;
    }

    if (nil != _state.device.cur_drawable) {
        [_state.device.cmd_buffer presentDrawable: _state.device.cur_drawable];
        _state.device.cur_drawable = nil;
    }
}

void gpu_commit() {
    assert(nil != _state.device.cmd_buffer);
    assert(nil == _state.device.cmd_encoder);

    [_state.device.cmd_buffer commit];
    _state.device.frame_index = (_state.device.frame_index + 1) % _state.device.frame_swap_count;
    _state.device.cmd_buffer = nil;
}

id<MTLLibrary> _mtl_library_from_bytecode(udata src) {
    NSError *err = nil;
    dispatch_data_t data = dispatch_data_create(src.data, src.length, nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    id<MTLLibrary> lib = [_state.device.device newLibraryWithData: data error: &err];
    if (nil == lib) {
        NSLog(@"Error: %@", err);
        NSLog(@"Source: %s", [err.localizedDescription UTF8String]);
    }
    dispatch_release(data);
    return lib;
}

id<MTLLibrary> _mtl_library_from_code(ustring src) {
    NSError *err = nil;
    id<MTLLibrary> lib = [_state.device.device
        newLibraryWithSource: [NSString stringWithUTF8String: src.data]
        options: nil
        error: &err];
    if (nil == lib) {
        NSLog(@"Error: %@", err);
        NSLog(@"Source: %s", [err.localizedDescription UTF8String]);
    }
    return lib;
}

gpu_shader gpu_create_shader(gpu_shader_desc *desc) {
    id<MTLLibrary> vertex_lib = nil;
    id<MTLLibrary> fragment_lib = nil;
    id<MTLFunction> vertex_func = nil;
    id<MTLFunction> fragment_func = nil;

    if (desc->vertex.bytecode.length > 0 && desc->fragment.bytecode.length > 0) {
        vertex_lib = _mtl_library_from_bytecode(desc->vertex.bytecode);
        fragment_lib = _mtl_library_from_bytecode(desc->fragment.bytecode);
        if (nil == vertex_lib || nil == fragment_lib) {
            goto failed;
        }
        vertex_func = [vertex_lib newFunctionWithName: [NSString stringWithUTF8String: desc->vertex.entry.data]];
        fragment_func = [fragment_lib newFunctionWithName: [NSString stringWithUTF8String:desc->fragment.entry.data]];
    } else if (desc->vertex.source.length > 0 && desc->fragment.source.length > 0) {
        vertex_lib = _mtl_library_from_code(desc->vertex.source);
        fragment_lib = _mtl_library_from_code(desc->fragment.source);
        if (nil == vertex_lib || nil == fragment_lib) {
            goto failed;
        }
        vertex_func = [vertex_lib newFunctionWithName: [NSString stringWithUTF8String: desc->vertex.entry.data]];
        fragment_func = [fragment_lib newFunctionWithName: [NSString stringWithUTF8String:desc->fragment.entry.data]];
    } else {
        goto failed;
    }

    if (nil == vertex_func) {
        ULOG_ERROR("Failed to create vertex function");
        goto failed;
    }

    if (nil == fragment_func) {
        ULOG_ERROR("Failed to create fragment function");
        goto failed;
    }

    gpu_shader_mtl_t mtl_shader = (gpu_shader_mtl_t){
        .vertex_lib = vertex_lib,
        .fragment_lib = fragment_lib,
        .vertex_func = vertex_func,
        .fragment_func = fragment_func,
    };
    return (gpu_shader){.id = _mtl_add_shader(mtl_shader)};

failed:
    if (vertex_lib) [vertex_lib release];
    if (fragment_lib) [fragment_lib release];
    if (vertex_func) [vertex_func release];
    if (fragment_func) [fragment_func release];
    return (gpu_shader){ .id = _mtl_invalid_id };
}

gpu_pipeline gpu_create_pipeline(gpu_pipeline_desc *desc) {
    MTLVertexDescriptor *vertex_desc = [MTLVertexDescriptor vertexDescriptor];
    gpu_pipeline_mtl_t mtl_pipeline;
    bool vertex_buffer_enabled[GPU_VERTEX_BUFFER_COUNT];
    memset(vertex_buffer_enabled, 0, sizeof(vertex_buffer_enabled));
    for (NSUInteger attr_index = 0; attr_index < GPU_ATTRIBUTE_COUNT; ++attr_index) {
        const gpu_vertex_attribute_state *attr_state = &desc->layout.attributes[attr_index];
        if (attr_state->format == ATTRIBUTE_FORMAT_INVALID) {
            break;
        }
        assert(attr_state->buffer_index < GPU_VERTEX_BUFFER_COUNT);
        vertex_desc.attributes[attr_index].format = _mtl_vertex_format(attr_state->format, attr_state->size);
        vertex_desc.attributes[attr_index].offset = attr_state->offset;
        vertex_desc.attributes[attr_index].bufferIndex = attr_state->buffer_index;
        vertex_buffer_enabled[attr_state->buffer_index] = true;
    }

    for (NSUInteger buffer_index = 0; buffer_index < GPU_VERTEX_BUFFER_COUNT; ++buffer_index) {
        if (!vertex_buffer_enabled[buffer_index]) continue;
        const gpu_vertex_buffer_layout_state *buffer_state = &desc->layout.buffers[buffer_index];
        assert(buffer_state->stride > 0);
        vertex_desc.layouts[buffer_index].stride = buffer_state->stride;
        vertex_desc.layouts[buffer_index].stepRate = buffer_state->step_rate;
        vertex_desc.layouts[buffer_index].stepFunction = _mtl_vertex_step_function(buffer_state->step_func);
        if (buffer_state->step_func == VERTEX_STEP_PER_INSTANCE) {
            mtl_pipeline.instanced = true;
        }
    }

    gpu_shader_mtl_t shader = _mtl_get_shader(desc->shader.id);
    MTLRenderPipelineDescriptor *pip_desc = [MTLRenderPipelineDescriptor new];
    pip_desc.vertexDescriptor = vertex_desc;
    pip_desc.vertexFunction = shader.vertex_func;
    pip_desc.fragmentFunction = shader.fragment_func;
    pip_desc.rasterSampleCount = desc->sample_count > 1 ? desc->sample_count : 1;
    pip_desc.alphaToCoverageEnabled = desc->alpha_to_coverage;
    pip_desc.alphaToOneEnabled = NO;
    pip_desc.rasterizationEnabled = YES;
    pip_desc.depthAttachmentPixelFormat = _mtl_pixel_format(desc->depth.format);
    if (desc->depth.format == PIXELFORMAT_DEPTH_STENCIL) {
        pip_desc.stencilAttachmentPixelFormat = _mtl_pixel_format(desc->depth.format);
    }

    for (NSUInteger i = 0; i < desc->color_count; ++i) {
        const gpu_color_target_state *color_state = &desc->colors[i];
        pip_desc.colorAttachments[i].pixelFormat = _mtl_pixel_format(color_state->format);
        pip_desc.colorAttachments[i].blendingEnabled = color_state->blend.enabled;
        pip_desc.colorAttachments[i].alphaBlendOperation = _mtl_blend_operation(color_state->blend.op_alpha);
        pip_desc.colorAttachments[i].rgbBlendOperation = _mtl_blend_operation(color_state->blend.op);
        pip_desc.colorAttachments[i].sourceRGBBlendFactor = _mtl_blend_factor(color_state->blend.src_factor);
        pip_desc.colorAttachments[i].destinationRGBBlendFactor = _mtl_blend_factor(color_state->blend.dst_factor);
        pip_desc.colorAttachments[i].sourceAlphaBlendFactor = _mtl_blend_factor(color_state->blend.src_factor_alpha);
        pip_desc.colorAttachments[i].destinationAlphaBlendFactor = _mtl_blend_factor(color_state->blend.dst_factor_alpha);
    }

    NSError *err = nil;
    id<MTLRenderPipelineState> pso = [_state.device.device newRenderPipelineStateWithDescriptor: pip_desc error: &err];
    [pip_desc release];

    if (nil == pso) {
        NSLog(@"Error: %@", err);
        NSLog(@"Source: %s", [err.localizedDescription UTF8String]);
        return (gpu_pipeline){ .id = _mtl_invalid_id };
    }
    mtl_pipeline.pso = pso;
    
    if (desc->depth.format != PIXELFORMAT_NONE) {
        MTLDepthStencilDescriptor *ds_desc = [MTLDepthStencilDescriptor new];
        ds_desc.depthCompareFunction = _mtl_compare_function(desc->depth.compare_func);
        ds_desc.depthWriteEnabled = desc->depth.write_enabled;
        
        if (desc->stencil.enabled) {
            const gpu_stencil_state *stencil = &desc->stencil;
            ds_desc.backFaceStencil = [MTLStencilDescriptor new];
            ds_desc.backFaceStencil.stencilFailureOperation = _mtl_stencil_operation(stencil->back.fail_op);
            ds_desc.backFaceStencil.depthFailureOperation = _mtl_stencil_operation(stencil->back.depth_fail_op);
            ds_desc.backFaceStencil.depthStencilPassOperation = _mtl_stencil_operation(stencil->back.pass_op);
            ds_desc.backFaceStencil.stencilCompareFunction = _mtl_compare_function(stencil->back.compare_func);
            ds_desc.backFaceStencil.readMask = stencil->read_mask;
            ds_desc.backFaceStencil.writeMask = stencil->write_mask;
            ds_desc.frontFaceStencil = [MTLStencilDescriptor new];
            ds_desc.frontFaceStencil.stencilFailureOperation = _mtl_stencil_operation(stencil->front.fail_op);
            ds_desc.frontFaceStencil.depthFailureOperation = _mtl_stencil_operation(stencil->front.depth_fail_op);
            ds_desc.frontFaceStencil.depthStencilPassOperation = _mtl_stencil_operation(stencil->front.pass_op);
            ds_desc.frontFaceStencil.stencilCompareFunction = _mtl_compare_function(stencil->front.compare_func);
            ds_desc.frontFaceStencil.readMask = stencil->read_mask;
            ds_desc.frontFaceStencil.writeMask = stencil->write_mask;
        }
        id<MTLDepthStencilState> dso = [_state.device.device newDepthStencilStateWithDescriptor: ds_desc];
        [ds_desc release];
        if (nil == dso) {
            return (gpu_pipeline){ .id = _mtl_invalid_id };
        }
        mtl_pipeline.dso = dso;
    }

    mtl_pipeline.depth_stencil_format = _mtl_pixel_format(desc->depth.format);
    mtl_pipeline.cull_mode = _mtl_cull_mode(desc->cull_mode);
    mtl_pipeline.winding = _mtl_winding(desc->face_winding);
    mtl_pipeline.primitive_type = _mtl_primitive_type(desc->primitive_type);
    mtl_pipeline.index_type = _mtl_index_type(desc->index_type);
    mtl_pipeline.indexed = desc->index_type != INDEX_NONE;
    mtl_pipeline.stencil_ref = desc->stencil.ref;
    return (gpu_pipeline){ .id = _mtl_add_pipeline(mtl_pipeline) };
}

void gpu_set_viewport(int x, int y, int width, int height) {
    assert(nil != _state.device.cmd_encoder);
    MTLViewport viewport = {
        .originX = (double)x,
        .originY = (double)y,
        .width = (double)width,
        .height = (double)height,
        .znear = 0.0,
        .zfar = 1.0,
    };
    [_state.device.cmd_encoder setViewport: viewport];
}

void gpu_set_scissor(int x, int y, int width, int height) {
    assert(nil != _state.device.cmd_encoder);
    MTLScissorRect scissor = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
    [_state.device.cmd_encoder setScissorRect: scissor];
}

void gpu_set_pipeline(gpu_pipeline pipeline) {
    assert(nil != _state.device.cmd_encoder);

    gpu_pipeline_mtl_t mtl_pipeline = _mtl_get_pipeline(pipeline.id);
    _state.cur_pipeline = mtl_pipeline;

    [_state.device.cmd_encoder setCullMode: mtl_pipeline.cull_mode];
    [_state.device.cmd_encoder setFrontFacingWinding: mtl_pipeline.winding];
    [_state.device.cmd_encoder setRenderPipelineState: mtl_pipeline.pso];
    if (mtl_pipeline.depth_stencil_format != MTLPixelFormatInvalid) {
        [_state.device.cmd_encoder setDepthStencilState: mtl_pipeline.dso];
    }
}

void gpu_set_binding(gpu_binding binding) {
    // assert(binding);
    // for (int i = 0; i < GPU_VERTEX_BUFFER_COUNT; ++i) {
    //     if (binding->buffers[i].id == _mtl_invalid_id) break;
    //     gpu_buffer_mtl_t buffer = _mtl_get_buffer(binding->buffers[i].id);
    //     NSUInteger offset = (NSUInteger)binding->buffer_offsets[i];
    //     [_state.device.cmd_encoder setVertexBuffer: buffer.buffer offset: offset atIndex: i];
    // }

    // if (binding->index_buffer.id != _mtl_invalid_id) {
    //     _state.cur_index_buffer = _mtl_get_buffer(binding->index_buffer.id);
    // }

    // for (int i = 0; i < GPU_SHADER_TEXTURE_COUNT; ++i) {
    //     if (binding->vertex.textures[i].id == _mtl_invalid_id) break;
    //     gpu_texture_mtl_t texture = _mtl_get_texture(binding->vertex.textures[i].id);
    //     [_state.device.cmd_encoder setVertexTexture: texture.texture atIndex: i];
    // }

    // for (int i = 0; i < GPU_SHADER_TEXTURE_COUNT; ++i) {
    //     if (binding->fragment.textures[i].id == _mtl_invalid_id) break;
    //     gpu_texture_mtl_t texture = _mtl_get_texture(binding->fragment.textures[i].id);
    //     [_state.device.cmd_encoder setFragmentTexture: texture.texture atIndex: i];
    // }
}

void gpu_draw(int base, int count, int instance_count) {
    if (_state.cur_pipeline.indexed) {
        [_state.device.cmd_encoder
            drawIndexedPrimitives: _state.cur_pipeline.primitive_type
            indexCount: count
            indexType: _state.cur_pipeline.index_type
            indexBuffer: _state.cur_index_buffer.buffer
            indexBufferOffset: 0
            instanceCount: instance_count];
    } else {
        [_state.device.cmd_encoder
            drawPrimitives: _state.cur_pipeline.primitive_type
            vertexStart: base
            vertexCount: count
            instanceCount: instance_count];
    }
}
