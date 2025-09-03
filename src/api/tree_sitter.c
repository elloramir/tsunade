#include <tree_sitter/api.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"

// --- você precisa declarar extern das linguagens que compilar
// exemplo: extern const TSLanguage *tree_sitter_lua(void);

typedef struct {
  TSParser *parser;
} LParser;

// Função Lua: treesitter.get_highlights(text, lang)
// Retorna: { { row=..., start_col=..., end_col=..., capture="..."}, ... }
static int l_get_highlights(lua_State *L) {
  size_t text_len;
  const char *source_code = luaL_checklstring(L, 1, &text_len);
  const char *lang_name   = luaL_checkstring(L, 2);

  // Escolher a linguagem (adicione os que quiser)
  const TSLanguage *lang = NULL;
  if (strcmp(lang_name, "lua") == 0) {
    extern const TSLanguage *tree_sitter_lua(void);
    lang = tree_sitter_lua();
  }
  else if (strcmp(lang_name, "c") == 0) {
    extern const TSLanguage *tree_sitter_c(void);
    lang = tree_sitter_c();
  }
  // TODO: adicione mais linguagens

  if (!lang) {
    luaL_error(L, "Unsupported language: %s", lang_name);
    return 0;
  }

  // criar parser
  TSParser *parser = ts_parser_new();
  ts_parser_set_language(parser, lang);

  // parse
  TSTree *tree = ts_parser_parse_string(
      parser, NULL, source_code, (uint32_t)text_len);

  TSNode root = ts_tree_root_node(tree);

  // --- agora criar query de highlights
  // normalmente vem do arquivo "highlights.scm" de cada grammar
  // aqui simplificado: ex: para Lua, você precisa carregar a query string
  extern const char *lua_highlights_query;
  uint32_t error_offset;
  TSQueryError error_type;

  TSQuery *query = ts_query_new(
      lang, lua_highlights_query,
      (uint32_t)strlen(lua_highlights_query),
      &error_offset, &error_type);

  if (!query) {
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    luaL_error(L, "failed to compile query at offset %u", error_offset);
    return 0;
  }

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root);

  lua_newtable(L); // results
  int idx = 1;

  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    for (uint32_t i = 0; i < match.capture_count; i++) {
      TSQueryCapture cap = match.captures[i];
      TSNode node = cap.node;
      const char *cap_name = ts_query_capture_name_for_id(query, cap.index, NULL);

      TSPoint start = ts_node_start_point(node);
      TSPoint end   = ts_node_end_point(node);

      lua_newtable(L);
      lua_pushinteger(L, (lua_Integer)start.row + 1);
      lua_setfield(L, -2, "row");
      lua_pushinteger(L, (lua_Integer)start.column + 1);
      lua_setfield(L, -2, "start_col");
      lua_pushinteger(L, (lua_Integer)end.column + 1);
      lua_setfield(L, -2, "end_col");
      lua_pushstring(L, cap_name);
      lua_setfield(L, -2, "capture");

      lua_rawseti(L, -2, idx++);
    }
  }

  ts_query_cursor_delete(cursor);
  ts_query_delete(query);
  ts_tree_delete(tree);
  ts_parser_delete(parser);

  return 1; // results table
}

static const luaL_Reg lib[] = {
  { "get_highlights", l_get_highlights },
  { NULL, NULL }
};

int luaopen_tree_sitter(lua_State *L) {
  luaL_newlib(L, lib);
  return 1;
}
