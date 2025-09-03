#ifndef API_H
#define API_H

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <stdio.h>

#define LUA_EXEC(L, code) do { \
    if (luaL_dostring(L, code) != LUA_OK) { \
        printf("LUA ERROR: %s\n", lua_tostring(L, -1)); \
        lua_pop(L, 1); \
    } \
} while(0)

#define LUA_EXEC_SILENT(L, code) do { \
    if (luaL_dostring(L, code) != LUA_OK) { \
        lua_pop(L, 1); \
    } \
} while(0)

#define LUA_CALL_MODULE_FUNC(L, module, func) \
    LUA_EXEC(L, "require('" module "')." func "()")

#define LUA_FUNC_EXISTS(L, module, func) ({ \
    bool exists = false; \
    lua_getglobal(L, "require"); \
    lua_pushstring(L, module); \
    if (lua_pcall(L, 1, 1, 0) == LUA_OK) { \
        lua_getfield(L, -1, func); \
        exists = lua_isfunction(L, -1); \
        lua_pop(L, 2); \
    } else { \
        lua_pop(L, 1); \
    } \
    exists; \
})

#define LUA_SET_GLOBAL_STRING(L, name, value) do { \
    lua_pushstring(L, value); \
    lua_setglobal(L, name); \
} while(0)

#define LUA_SET_GLOBAL_NUMBER(L, name, value) do { \
    lua_pushnumber(L, value); \
    lua_setglobal(L, name); \
} while(0)

#define LUA_SET_ARGS_TABLE(L, argc, argv) do { \
    lua_newtable(L); \
    for (int i = 0; i < argc; i++) { \
        lua_pushstring(L, argv[i]); \
        lua_rawseti(L, -2, i + 1); \
    } \
    lua_setglobal(L, "ARGS"); \
} while(0)

#ifdef _WIN32
    #define PLATFORM_NAME "Windows"
#elif __linux__
    #define PLATFORM_NAME "Linux"
#elif __APPLE__
    #define PLATFORM_NAME "macOS"
#else
    #define PLATFORM_NAME "Unknown"
#endif

#define LUA_SETUP_GLOBALS(L, argc, argv, version, exe_path) do { \
    LUA_SET_ARGS_TABLE(L, argc, argv); \
    LUA_SET_GLOBAL_STRING(L, "VERSION", version); \
    LUA_SET_GLOBAL_STRING(L, "PLATFORM", PLATFORM_NAME); \
    LUA_SET_GLOBAL_NUMBER(L, "SCALE", 1); \
    LUA_SET_GLOBAL_STRING(L, "EXEFILE", exe_path); \
} while(0)

#define LUA_INIT_CORE(L) \
    LUA_EXEC(L, \
        "local core\n" \
        "xpcall(function()\n" \
        "  SCALE = tonumber(os.getenv('LITE_SCALE')) or SCALE\n" \
        "  PATHSEP = package.config:sub(1, 1)\n" \
        "  EXEDIR = EXEFILE:match('^(.+)[/\\\\].*$')\n" \
        "  package.path = EXEDIR .. '/data/?.lua;' .. package.path\n" \
        "  package.path = EXEDIR .. '/data/?/init.lua;' .. package.path\n" \
        "  core = require('core')\n" \
        "  core.init()\n" \
        "end, function(err)\n" \
        "  print('Error: ' .. tostring(err))\n" \
        "  print(debug.traceback(nil, 2))\n" \
        "  if core and core.on_error then\n" \
        "    pcall(core.on_error, err)\n" \
        "  end\n" \
        "  os.exit(1)\n" \
        "end)")

#define API_TYPE_FONT "Font"

typedef struct sapp_event sapp_event;

void api_load_libs(lua_State *L);
void enqueue_event(const sapp_event* e);

#endif
