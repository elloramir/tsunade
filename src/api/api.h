#ifndef API_H
#define API_H

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#define LUA_MODULE_CALL(L, module, func) \
do { \
    lua_getglobal(L, "require"); \
    lua_pushstring(L, module); \
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) { \
        fprintf(stderr, "[Lua Error] require('%s'): %s\n", module, lua_tostring(L, -1)); \
        lua_pop(L, 1); \
    } else { \
        lua_getfield(L, -1, func); \
        lua_remove(L, -2); \
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) { \
            fprintf(stderr, "[Lua Error] %s.%s(): %s\n", module, func, lua_tostring(L, -1)); \
            lua_pop(L, 1); \
        } \
    } \
} while(0)

typedef struct sapp_event sapp_event;

#define API_TYPE_FONT "Font"

void api_load_libs(lua_State *L);
void enqueue_event(const sapp_event* e);

#endif
