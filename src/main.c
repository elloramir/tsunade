#define SOKOL_APP_IMPL
#include <sokol_app.h>
#include "renderer.h"
#define SOKOL_LOG_IMPL
#include <sokol_log.h>
#define SOKOL_GLUE_IMPL
#include <sokol_glue.h>

#include "api/api.h"

#include <stdio.h>
#include <string.h>
#if __linux__ || __APPLE__
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

static struct {
    int argc;
    char **argv;
    lua_State *L;
} state;

static void get_exe_filename(char *buf, int sz) {
#if _WIN32
    int len = GetModuleFileName(NULL, buf, sz - 1);
    buf[len] = '\0';
#elif __linux__
    char path[512];
    sprintf(path, "/proc/%d/exe", getpid());
    int len = readlink(path, buf, sz - 1);
    buf[len] = '\0';
#elif __APPLE__
    unsigned size = sz;
    _NSGetExecutablePath(buf, &size);
#else
    strcpy(buf, "./tsunade");
#endif
}

static void init(void) {
    ren_init();

    state.L = luaL_newstate();
    luaL_openlibs(state.L);
    api_load_libs(state.L);

    char exe_filename[1024];
    get_exe_filename(exe_filename, sizeof(exe_filename));

    LUA_SETUP_GLOBALS(state.L, state.argc, state.argv, "1.11", exe_filename);
    LUA_INIT_CORE(state.L);
}

static void frame(void) {
    LUA_CALL_MODULE_FUNC(state.L, "core", "run");
}

static void cleanup(void) {
    LUA_CALL_MODULE_FUNC(state.L, "core", "quit");
    
    if (state.L) {
        lua_close(state.L);
        state.L = NULL;
    }
    
    sfons_destroy();
    sgl_shutdown();
    sg_shutdown();
}

static void event(const sapp_event* e) {
    enqueue_event(e);
}

sapp_desc sokol_main(int param_argc, char* param_argv[]) {
    state.argc = param_argc;
    state.argv = param_argv;

    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 960,
        .height = 640,
        .window_title = "AKI text editor",
        .logger.func = slog_func,
    };
}