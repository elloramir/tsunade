#define SOKOL_APP_IMPL
#include <sokol_app.h>
#include "renderer.h"
#define SOKOL_LOG_IMPL
#include <sokol_log.h>
#define SOKOL_GLUE_IMPL
#include <sokol_glue.h>

#include "api/api.h"

static struct
{
    int argc;
    char **argv;
    lua_State *L;
}
state;

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

    lua_pushnumber(state.L, 1.5);
    lua_setglobal(state.L, "SCALE");

    lua_pushstring(state.L, "/home/lucas/pedro/tsunade/tsunade");
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
    (void)luaL_dostring(state.L, "require('core').run()");
}

static void cleanup(void) {
    // @todo(ellora): Investigate why this event was not trigged from the event queue
    (void)luaL_dostring(state.L, "require('core').quit()");
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
        .width = 800,
        .height = 600,
        .window_title = "AKI text editor",
        .logger.func = slog_func,
    };
}