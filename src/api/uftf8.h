#ifndef UTF8_H
#define UTF8_H

#include <inttypes.h>
#include <stdbool.h>

int utf8_encode(uint32_t codepoint, char* buffer);
int utf8_decode(const char* str, uint32_t* codepoint, int* bytes_consumed);
bool is_valid_utf8(const char* str);
size_t utf8_strlen(const char* str);
int utf8_fuzzy_match(const char* str, const char* pattern);

#endif