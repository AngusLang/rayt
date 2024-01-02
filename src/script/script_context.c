#include "script/script.h"
#include <quickjs/quickjs-libc.h>
#include <quickjs/quickjs.h>

typedef struct qjs_module {
    JSRuntime *runtime;
    JSContext *context;
} qjs_module;

static script_context_t shared_context = {0};
static qjs_module shared_module = {0};

void script_context_init(GLFWwindow *window) {
    shared_context.module = (void *)&shared_module;
    shared_context.ui_scale = 2.0f;
    shared_module.runtime = JS_NewRuntime();
    shared_module.context = JS_NewContext(shared_module.runtime);
}

script_context_t *script_context_share(void) { return &shared_context; }
void *script_context_internal(void) { return shared_module.context; }
void *script_runtime_internal(void) { return shared_module.runtime; }

void script_context_destroy(void) {
    if (shared_module.context == NULL)
        return;
    JS_FreeContext(shared_module.context);
    shared_module.context = NULL;
    JS_RunGC(shared_module.runtime);
    // JS_FreeRuntime(context->runtime);
}

void script_context_cleanup(void) {
    script_context_share();
    script_context_destroy();

    shared_module.runtime = JS_NewRuntime();
    shared_module.context = JS_NewContext(shared_module.runtime);
}

int script_eval(ustring source, ustring_view filename) {
    int ret;

    if (source.length == 0) {
        printf("source is empty\n");
        return -1;
    }

    script_module_browser_cleanup();
    script_context_cleanup();
    script_module_browser_register();
    script_module_webgl2_register();
    JSContext *ctx = script_context_internal();
    JSValue val = JS_Eval(ctx, source.data, source.length, filename.base.data, 0);

    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }

    JS_FreeValue(ctx, val);
    return ret;
}


void script_loop_tick() {
    if (script_context_internal() == NULL) return;
    int finished;
    JSContext *ctx;
    while((finished = JS_ExecutePendingJob(script_runtime_internal(), &ctx)) != 0) {
        if (finished < 0) {
            js_std_dump_error(script_context_internal());
            break;
        }
    }
}
