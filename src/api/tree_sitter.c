#include <tree_sitter/api.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"

static const luaL_Reg lib[] = {
  { NULL, NULL }
};

int luaopen_tree_sitter(lua_State *L) {
  luaL_newlib(L, lib);
  return 1;
}
