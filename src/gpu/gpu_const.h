#pragma once

enum {
    GPU_SWAP_BUFFER_COUNT = 3,
    GPU_ATTRIBUTE_COUNT = 16,
    GPU_ATTACHMENT_COUNT = 4,
    GPU_BLOCK_UNIFORM_COUNT = 16,
    GPU_BLOCK_COUNT = 4,
    GPU_SHADER_TEXTURE_COUNT = 12,
    GPU_SHADER_SAMPLER_COUNT = 8,
};

typedef enum gpu_pixel_format {
    _PIXELFORMAT_DEFAULT,    // value 0 reserved for default-init
    PIXELFORMAT_NONE,

    PIXELFORMAT_R8,
    PIXELFORMAT_R8SN,
    PIXELFORMAT_R8UI,
    PIXELFORMAT_R8SI,

    PIXELFORMAT_R16,
    PIXELFORMAT_R16SN,
    PIXELFORMAT_R16UI,
    PIXELFORMAT_R16SI,
    PIXELFORMAT_R16F,
    PIXELFORMAT_RG8,
    PIXELFORMAT_RG8SN,
    PIXELFORMAT_RG8UI,
    PIXELFORMAT_RG8SI,

    PIXELFORMAT_R32UI,
    PIXELFORMAT_R32SI,
    PIXELFORMAT_R32F,
    PIXELFORMAT_RG16,
    PIXELFORMAT_RG16SN,
    PIXELFORMAT_RG16UI,
    PIXELFORMAT_RG16SI,
    PIXELFORMAT_RG16F,
    PIXELFORMAT_RGBA8,
    PIXELFORMAT_SRGB8A8,
    PIXELFORMAT_RGBA8SN,
    PIXELFORMAT_RGBA8UI,
    PIXELFORMAT_RGBA8SI,
    PIXELFORMAT_BGRA8,
    PIXELFORMAT_RGB10A2,
    PIXELFORMAT_RG11B10F,
    PIXELFORMAT_RGB9E5,

    PIXELFORMAT_RG32UI,
    PIXELFORMAT_RG32SI,
    PIXELFORMAT_RG32F,
    PIXELFORMAT_RGBA16,
    PIXELFORMAT_RGBA16SN,
    PIXELFORMAT_RGBA16UI,
    PIXELFORMAT_RGBA16SI,
    PIXELFORMAT_RGBA16F,

    PIXELFORMAT_RGBA32UI,
    PIXELFORMAT_RGBA32SI,
    PIXELFORMAT_RGBA32F,

    // NOTE: when adding/removing pixel formats before DEPTH, also update sokol_app.h/_SAPP_PIXELFORMAT_*
    PIXELFORMAT_DEPTH,
    PIXELFORMAT_DEPTH_STENCIL,

    // NOTE: don't put any new compressed format in front of here
    PIXELFORMAT_BC1_RGBA,
    PIXELFORMAT_BC2_RGBA,
    PIXELFORMAT_BC3_RGBA,
    PIXELFORMAT_BC3_SRGBA,
    PIXELFORMAT_BC4_R,
    PIXELFORMAT_BC4_RSN,
    PIXELFORMAT_BC5_RG,
    PIXELFORMAT_BC5_RGSN,
    PIXELFORMAT_BC6H_RGBF,
    PIXELFORMAT_BC6H_RGBUF,
    PIXELFORMAT_BC7_RGBA,
    PIXELFORMAT_BC7_SRGBA,
    PIXELFORMAT_PVRTC_RGB_2BPP,      // FIXME: deprecated
    PIXELFORMAT_PVRTC_RGB_4BPP,      // FIXME: deprecated
    PIXELFORMAT_PVRTC_RGBA_2BPP,     // FIXME: deprecated
    PIXELFORMAT_PVRTC_RGBA_4BPP,     // FIXME: deprecated
    PIXELFORMAT_ETC2_RGB8,
    PIXELFORMAT_ETC2_SRGB8,
    PIXELFORMAT_ETC2_RGB8A1,
    PIXELFORMAT_ETC2_RGBA8,
    PIXELFORMAT_ETC2_SRGB8A8,
    PIXELFORMAT_ETC2_RG11,
    PIXELFORMAT_ETC2_RG11SN,

    PIXELFORMAT_ASTC_4x4_RGBA,
    PIXELFORMAT_ASTC_4x4_SRGBA,

    _PIXELFORMAT_NUM,
} gpu_pixel_format;

typedef enum gpu_buffer_type {
    BUFFER_VERTEX,
    BUFFER_INDEX,
    BUFFER_UNIFORM,
    BUFFER_STORAGE
} gpu_buffer_type;

typedef enum gpu_texture_type {
    TEXTURE_2D,
    TEXTURE_CUBE,
    TEXTURE_3D,
    TEXTURE_ARRAY,
} gpu_texture_type;

typedef enum gpu_attribute_type {
    ATTRIBUTE_FLOAT,
    ATTRIBUTE_UINT,
    ATTRIBUTE_INT,
    ATTRIBUTE_USHORT,
    ATTRIBUTE_SHORT,
    ATTRIBUTE_BYTE,
    ATTRIBUTE_UBYTE,
} gpu_attribute_type;

typedef enum gpu_uniform_layout {
    UNIFORM_LAYOUT_AUTO,
    UNIFORM_LAYOUT_NATIVE,
    UNIFORM_LAYOUT_STD140,
} gpu_uniform_layout;

typedef enum gpu_uniform_type {
    UNIFORM_FLOAT,
    UNIFORM_FLOAT2,
    UNIFORM_FLOAT3,
    UNIFORM_FLOAT4,
    UNIFORM_MAT3,
    UNIFORM_MAT4,
} gpu_uniform_type;

typedef enum gpu_texture_sample_type {
    TEXTURE_SAMPLE_TYPE_FLOAT,
    TEXTURE_SAMPLE_TYPE_DEPTH,
    TEXTURE_SAMPLE_TYPE_SINT,
    TEXTURE_SAMPLE_TYPE_UINT,
    TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT,
} gpu_texture_sample_type;

typedef enum gpu_sampler_type {
    SAMPLER_TYPE_FILTERING,
    SAMPLER_TYPE_NEAREST,
    SAMPLER_TYPE_COMPARISON,
} gpu_sampler_type;

typedef enum gpu_filter {
    FILTER_AUTO,
    FILTER_NEAREST,
    FILTER_LINEAR,
} gpu_filter;

typedef enum gpu_wrap {
    WRAP_AUTO,
    WRAP_CLAMP_TO_EDGE,
    WRAP_REPEAT,
    WRAP_MIRRORED_REPEAT,
} gpu_wrap;

typedef enum gpu_compare_func {
    COMPARE_AUTO,
    COMPARE_NEVER,
    COMPARE_LESS,
    COMPARE_EQUAL,
    COMPARE_LESS_EQUAL,
    COMPARE_GREATER,
    COMPARE_NOT_EQUAL,
    COMPARE_GREATER_EQUAL,
    COMPARE_ALWAYS,
} gpu_compare_func;

typedef enum gpu_primitive_type {
    PRIMITIVE_POINTS,
    PRIMITIVE_LINES,
    PRIMITIVE_LINE_STRIP,
    PRIMITIVE_LINE_LOOP,
    PRIMITIVE_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP,
    PRIMITIVE_TRIANGLE_FAN,
} gpu_primitive_type;

typedef enum gpu_index_type {
    INDEX_UINT16,
    INDEX_UINT32,
} gpu_index_type;

typedef enum gpu_face_winding {
    FACE_WINDING_CCW,
    FACE_WINDING_CW,
} gpu_face_winding;

typedef enum gpu_cull_mode {
    CULL_NONE,
    CULL_FRONT,
    CULL_BACK,
} gpu_cull_mode;

typedef enum gpu_load_action {
    LOAD_ACTION_CLEAR,
    LOAD_ACTION_LOAD,
    LOAD_ACTION_DONTCARE,
} gpu_load_action;

typedef enum gpu_store_action {
    STORE_ACTION_STORE,
    STORE_ACTION_DONTCARE,
} gpu_store_action;

enum {
    GPU_BUFFER_POOL_SIZE = 128,
    GPU_TEXTURE_POOL_SIZE = 128,
    GPU_SHADER_POOL_SIZE = 64,
    GPU_SAMPLER_POOL_SIZE = 64,
    GPU_PIPELINE_POOL_SIZE = 64,
    GPU_ATTACHMENTS_POOL_SIZE = 16,
    GPU_UB_SIZE = 4 * 1024 * 1024,
};