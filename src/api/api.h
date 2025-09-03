#ifndef API_H
#define API_H

#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

typedef struct sapp_event sapp_event;

#define API_TYPE_FONT "Font"

void api_load_libs(lua_State *L);
void enqueue_event(const sapp_event* e);

#endif
