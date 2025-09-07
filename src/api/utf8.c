#include <ctype.h>
#include "uftf8.h"

int utf8_encode(uint32_t codepoint, char* buffer) {
    if (codepoint <= 0x7F) {
        // ASCII (0-127)
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        // 2 bytes UTF-8
        buffer[0] = (char)(0xC0 | (codepoint >> 6));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint <= 0xFFFF) {
        // 3 bytes UTF-8
        buffer[0] = (char)(0xE0 | (codepoint >> 12));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        // 4 bytes UTF-8
        buffer[0] = (char)(0xF0 | (codepoint >> 18));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }
    return 0;
}

int utf8_decode(const char* str, uint32_t* codepoint, int* bytes_consumed) {
    if (!str || !codepoint || !bytes_consumed) return 0;
    
    unsigned char c = (unsigned char)str[0];
    *bytes_consumed = 0;
    
    if (c <= 0x7F) {
        // ASCII
        *codepoint = c;
        *bytes_consumed = 1;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        // 2 bytes
        if ((str[1] & 0xC0) != 0x80) return 0;
        *codepoint = ((c & 0x1F) << 6) | (str[1] & 0x3F);
        *bytes_consumed = 2;
        return 1;
    } else if ((c & 0xF0) == 0xE0) {
        // 3 bytes
        if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80) return 0;
        *codepoint = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
        *bytes_consumed = 3;
        return 1;
    } else if ((c & 0xF8) == 0xF0) {
        // 4 bytes
        if ((str[1] & 0xC0) != 0x80 || (str[2] & 0xC0) != 0x80 || (str[3] & 0xC0) != 0x80) return 0;
        *codepoint = ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
        *bytes_consumed = 4;
        return 1;
    }
    
    return 0;
}

bool is_valid_utf8(const char* str) {
    uint32_t codepoint;
    int bytes_consumed;
    
    while (*str) {
        if (!utf8_decode(str, &codepoint, &bytes_consumed)) {
            return false;
        }
        str += bytes_consumed;
    }
    return true;
}

size_t utf8_strlen(const char* str) {
    size_t count = 0;
    uint32_t codepoint;
    int bytes_consumed;
    
    while (*str) {
        if (utf8_decode(str, &codepoint, &bytes_consumed)) {
            count++;
            str += bytes_consumed;
        } else {
            count++;
            str++;
        }
    }
    return count;
}

int utf8_fuzzy_match(const char* str, const char* pattern) {
    int score = 0;
    int run = 0;
    uint32_t str_char, ptn_char;
    int str_bytes, ptn_bytes;
    
    while (*str && *pattern) {
        while (*str == ' ') str++;
        while (*pattern == ' ') pattern++;
        
        if (!*str || !*pattern) break;
        
        if (!utf8_decode(str, &str_char, &str_bytes)) {
            str_char = (unsigned char)*str;
            str_bytes = 1;
        }
        
        if (!utf8_decode(pattern, &ptn_char, &ptn_bytes)) {
            ptn_char = (unsigned char)*pattern;
            ptn_bytes = 1;
        }

        uint32_t str_lower = str_char;
        uint32_t ptn_lower = ptn_char;
        
        if (str_char <= 127) str_lower = tolower(str_char);
        if (ptn_char <= 127) ptn_lower = tolower(ptn_char);
        
        if (str_lower == ptn_lower) {
            score += run * 10 - (str_char != ptn_char ? 1 : 0);
            run++;
            pattern += ptn_bytes;
        } else {
            score -= 10;
            run = 0;
        }
        str += str_bytes;
    }
    
    if (*pattern) return -1;
    
    size_t remaining = utf8_strlen(str);
    return score - (int)remaining;
}

