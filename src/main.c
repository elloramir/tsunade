
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

static struct
{
    int argc;
    char **argv;
    lua_State *L;
}
state;

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

    // Pass command-line arguments to the Lua state.
    lua_newtable(state.L);
    for (int i = 0; i < state.argc; i++) {
        lua_pushstring(state.L, state.argv[i]);
        lua_rawseti(state.L, -2, i + 1);
    }
    lua_setglobal(state.L, "ARGS");

    // Define global variables in the Lua state.
    lua_pushstring(state.L, "1.11");
    lua_setglobal(state.L, "VERSION");

    lua_pushstring(state.L, "Windows");
    lua_setglobal(state.L, "PLATFORM");

    lua_pushnumber(state.L, 1);
    lua_setglobal(state.L, "SCALE");

    char exe_buffer[1024];
    get_exe_filename(exe_buffer, sizeof(exe_buffer));
    lua_pushstring(state.L, exe_buffer);
    lua_setglobal(state.L, "EXEFILE");

    // Execute the Lua initialization script.
    (void)luaL_dostring(state.L,
        "local core\n"
        "xpcall(function()\n"
        "  SCALE = tonumber(os.getenv(\"LITE_SCALE\")) or SCALE\n"
        "  PATHSEP = package.config:sub(1, 1)\n"
        "  EXEDIR = EXEFILE:match(\"^(.+)[/\\\\].*$\")\n"
        "  package.path = EXEDIR .. '/data/?.lua;' .. package.path\n"
        "  package.path = EXEDIR .. '/data/?/init.lua;' .. package.path\n"
        "  core = require('core')\n"
        "  core.init()\n"
        "end, function(err)\n"
        "  print('Error: ' .. tostring(err))\n"
        "  print(debug.traceback(nil, 2))\n"
        "  if core and core.on_error then\n"
        "    pcall(core.on_error, err)\n"
        "  end\n"
        "  os.exit(1)\n"
        "end)");
}

static void frame(void) {
    LUA_MODULE_CALL(state.L, "core", "run");
}

static void cleanup(void) {
    // @todo(ellora): Investigate why this event was not trigged from the event queue
    LUA_MODULE_CALL(state.L, "core", "quit");
    lua_close(state.L);
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
