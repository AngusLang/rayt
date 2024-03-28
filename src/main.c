#include "foundation/api.h"
#include "os/os.h"
#include "script/api.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#include <uv.h>

void on_launch(os_window_t* window) {
    logger_init(logger_global());
    script_context_init(window);
    // script_eval_uri(uri);
}

void on_frame(os_window_t* window) {
    script_context_loop_tick();
}

void on_terminate(os_window_t* window) {
    logger_destroy(logger_global());
    script_context_cleanup();
    script_context_terminate();
    os_window_close(window);
}

int main(int argc, char** argv) {
    int window_width = 1080;
    int window_height = 720;
    os_setup(argc, argv);
    os_window_create(ustring_STR("union_native"), window_width, window_height, on_launch, on_frame, on_terminate);
    return 0;
}
