#include <sokol_app.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define realpath(x, y) _fullpath(y, x, MAX_PATH)
#endif

#include "api.h"
#include "uftf8.h"

#define MAX_EVENTS 128

static struct
{
    sapp_event event_queue[MAX_EVENTS];
    int queue_head;
    int queue_tail;

    double last_click_time;
    float last_click_x;
    float last_click_y;
    sapp_mousebutton last_click_button;
    int click_count;
    
    int last_width;
    int last_height;
    bool has_focus;
}
state;

static double time_now(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

void enqueue_event(const sapp_event* e) {
    int next = (state.queue_tail + 1) % MAX_EVENTS;
    if (next != state.queue_head) {
        state.event_queue[state.queue_tail] = *e;
        state.queue_tail = next;
    }
}

static bool dequeue_event(sapp_event* out) {
    if (state.queue_head == state.queue_tail) return false;
    *out = state.event_queue[state.queue_head];
    state.queue_head = (state.queue_head + 1) % MAX_EVENTS;
    return true;
}

static int get_click_count(const sapp_event* e) {
    if (e->type != SAPP_EVENTTYPE_MOUSE_DOWN) return 0;

    double now = time_now();
    double dt = now - state.last_click_time;

    bool same_button = (e->mouse_button == state.last_click_button);
    bool close_pos = (fabsf(e->mouse_x - state.last_click_x) < 4 &&
                      fabsf(e->mouse_y - state.last_click_y) < 4);

    if (same_button && close_pos && dt < 0.3) {
        state.click_count++;
    } else {
        state.click_count = 1;
    }

    state.last_click_time = now;
    state.last_click_x = e->mouse_x;
    state.last_click_y = e->mouse_y;
    state.last_click_button = e->mouse_button;

    return state.click_count;
}

static const char* button_name(int button) {
    switch (button) {
    case 1: return "left";
    case 2: return "middle";
    case 3: return "right";
    default: return "?";
    }
}

static char* key_name(char* dst, int sym) {
    switch (sym) {
    case SAPP_KEYCODE_SPACE: strcpy(dst, "space"); break;
    case SAPP_KEYCODE_ENTER: strcpy(dst, "return"); break;
    case SAPP_KEYCODE_TAB: strcpy(dst, "tab"); break;
    case SAPP_KEYCODE_BACKSPACE: strcpy(dst, "backspace"); break;
    case SAPP_KEYCODE_ESCAPE: strcpy(dst, "escape"); break;

    case SAPP_KEYCODE_LEFT_SHIFT: strcpy(dst, "left shift"); break;
    case SAPP_KEYCODE_RIGHT_SHIFT: strcpy(dst, "right shift"); break;
    case SAPP_KEYCODE_LEFT_CONTROL: strcpy(dst, "left ctrl"); break;
    case SAPP_KEYCODE_RIGHT_CONTROL: strcpy(dst, "right ctrl"); break;
    case SAPP_KEYCODE_LEFT_ALT: strcpy(dst, "left alt"); break;
    case SAPP_KEYCODE_RIGHT_ALT: strcpy(dst, "right alt"); break;
    case SAPP_KEYCODE_LEFT_SUPER: strcpy(dst, "left super"); break;
    case SAPP_KEYCODE_RIGHT_SUPER: strcpy(dst, "right super"); break;
    case SAPP_KEYCODE_MENU: strcpy(dst, "menu"); break;

    case SAPP_KEYCODE_CAPS_LOCK: strcpy(dst, "caps lock"); break;
    case SAPP_KEYCODE_NUM_LOCK: strcpy(dst, "num lock"); break;
    case SAPP_KEYCODE_SCROLL_LOCK: strcpy(dst, "scroll lock"); break;

    case SAPP_KEYCODE_LEFT: strcpy(dst, "left"); break;
    case SAPP_KEYCODE_RIGHT: strcpy(dst, "right"); break;
    case SAPP_KEYCODE_UP: strcpy(dst, "up"); break;
    case SAPP_KEYCODE_DOWN: strcpy(dst, "down"); break;
    case SAPP_KEYCODE_HOME: strcpy(dst, "home"); break;
    case SAPP_KEYCODE_END: strcpy(dst, "end"); break;
    case SAPP_KEYCODE_PAGE_DOWN: strcpy(dst, "page down"); break;
    case SAPP_KEYCODE_PAGE_UP: strcpy(dst, "page up"); break;
    case SAPP_KEYCODE_INSERT: strcpy(dst, "insert"); break;
    case SAPP_KEYCODE_DELETE: strcpy(dst, "delete"); break;

    case SAPP_KEYCODE_PRINT_SCREEN: strcpy(dst, "print screen"); break;
    case SAPP_KEYCODE_PAUSE: strcpy(dst, "pause"); break;

    case SAPP_KEYCODE_F1: case SAPP_KEYCODE_F2: case SAPP_KEYCODE_F3:
    case SAPP_KEYCODE_F4: case SAPP_KEYCODE_F5: case SAPP_KEYCODE_F6:
    case SAPP_KEYCODE_F7: case SAPP_KEYCODE_F8: case SAPP_KEYCODE_F9:
    case SAPP_KEYCODE_F10: case SAPP_KEYCODE_F11: case SAPP_KEYCODE_F12:
    case SAPP_KEYCODE_F13: case SAPP_KEYCODE_F14: case SAPP_KEYCODE_F15:
    case SAPP_KEYCODE_F16: case SAPP_KEYCODE_F17: case SAPP_KEYCODE_F18:
    case SAPP_KEYCODE_F19: case SAPP_KEYCODE_F20: case SAPP_KEYCODE_F21:
    case SAPP_KEYCODE_F22: case SAPP_KEYCODE_F23: case SAPP_KEYCODE_F24:
        sprintf(dst, "F%d", 1 + (sym - SAPP_KEYCODE_F1));
        break;

    case SAPP_KEYCODE_0: case SAPP_KEYCODE_1: case SAPP_KEYCODE_2:
    case SAPP_KEYCODE_3: case SAPP_KEYCODE_4: case SAPP_KEYCODE_5:
    case SAPP_KEYCODE_6: case SAPP_KEYCODE_7: case SAPP_KEYCODE_8:
    case SAPP_KEYCODE_9:
        sprintf(dst, "%c", '0' + (sym - SAPP_KEYCODE_0));
        break;

    case SAPP_KEYCODE_A: case SAPP_KEYCODE_B: case SAPP_KEYCODE_C:
    case SAPP_KEYCODE_D: case SAPP_KEYCODE_E: case SAPP_KEYCODE_F:
    case SAPP_KEYCODE_G: case SAPP_KEYCODE_H: case SAPP_KEYCODE_I:
    case SAPP_KEYCODE_J: case SAPP_KEYCODE_K: case SAPP_KEYCODE_L:
    case SAPP_KEYCODE_M: case SAPP_KEYCODE_N: case SAPP_KEYCODE_O:
    case SAPP_KEYCODE_P: case SAPP_KEYCODE_Q: case SAPP_KEYCODE_R:
    case SAPP_KEYCODE_S: case SAPP_KEYCODE_T: case SAPP_KEYCODE_U:
    case SAPP_KEYCODE_V: case SAPP_KEYCODE_W: case SAPP_KEYCODE_X:
    case SAPP_KEYCODE_Y: case SAPP_KEYCODE_Z:
        sprintf(dst, "%c", 'a' + (sym - SAPP_KEYCODE_A));
        break;

    case SAPP_KEYCODE_MINUS: strcpy(dst, "-"); break;
    case SAPP_KEYCODE_EQUAL: strcpy(dst, "="); break;

    default:
        sprintf(dst, "?(%d)", sym);
        break;
    }
    return dst;
}

static const char* cursor_opts[] = {
    "arrow", "ibeam", "sizeh",
    "sizev", "hand", NULL
};

static const sapp_mouse_cursor cursor_enums[] = {
    SAPP_MOUSECURSOR_ARROW,
    SAPP_MOUSECURSOR_IBEAM,
    SAPP_MOUSECURSOR_RESIZE_EW,
    SAPP_MOUSECURSOR_RESIZE_NS,
    SAPP_MOUSECURSOR_POINTING_HAND
};

static int f_set_cursor(lua_State* L) {
    int opt = luaL_checkoption(L, 1, "arrow", cursor_opts);
    sapp_mouse_cursor n = cursor_enums[opt];
    sapp_set_mouse_cursor(n);
    return 0;
}

static int f_set_window_title(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    sapp_set_window_title(title);
    return 0;
}

static int f_set_window_mode(lua_State *L) {
    (void)L;
    return 0;
}

static int f_window_has_focus(lua_State* L) {
    lua_pushboolean(L, state.has_focus);
    return 1;
}

static int f_show_confirm_dialog(lua_State *L) {
    (void)L;
    return 1;
}

static int f_get_time(lua_State* L) {
    lua_pushnumber(L, time_now());
    return 1;
}

static int f_chdir(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int err = chdir(path);
    if (err) { luaL_error(L, "chdir() failed: %s", strerror(errno)); }
    return 0;
}

static int f_list_dir(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    DIR* dir = opendir(path);
    if (!dir) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_newtable(L);
    int i = 1;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        lua_pushstring(L, entry->d_name);
        lua_rawseti(L, -2, i++);
    }
    closedir(dir);
    return 1;
}

static int f_get_file_info(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    struct stat s;
    if (stat(path, &s) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_newtable(L);
    lua_pushnumber(L, s.st_mtime);
    lua_setfield(L, -2, "modified");
    lua_pushnumber(L, s.st_size);
    lua_setfield(L, -2, "size");
    if (S_ISREG(s.st_mode)) {
        lua_pushstring(L, "file");
    } else if (S_ISDIR(s.st_mode)) {
        lua_pushstring(L, "dir");
    } else {
        lua_pushnil(L);
    }
    lua_setfield(L, -2, "type");
    return 1;
}

static int f_poll_event(lua_State* L) {
    sapp_event e;
    char buf[16];

    if (!dequeue_event(&e)) {
        // @note(ellora): Sokol does not have maximized event, so we
        // need to do that for now \_(ツ)_/
        {
            int width = sapp_width();
            int height = sapp_height();
            if (width != state.last_width || height != state.last_height) {
                lua_pushstring(L, "maximized");
                lua_pushnumber(L, width);
                lua_pushnumber(L, height);
                state.last_width = width;
                state.last_height = height;
                return 3;
            }
        }

        return 0;
    }

    switch (e.type) {

    // --- Window / System ---
    case SAPP_EVENTTYPE_QUIT_REQUESTED:
        lua_pushstring(L, "quit");
        return 1;

    case SAPP_EVENTTYPE_RESIZED:
        lua_pushstring(L, "resized");
        lua_pushnumber(L, e.framebuffer_width);
        lua_pushnumber(L, e.framebuffer_height);
        return 3;

    case SAPP_EVENTTYPE_FOCUSED:
        state.has_focus = true;
        lua_pushstring(L, "focused");
        return 1;

    case SAPP_EVENTTYPE_UNFOCUSED: {
        // @todo(ellora): That is not the ideal, but...
        // Fix stuck modifier keys (alt/ctrl/shift) when losing focus
        sapp_event fake;
        memset(&fake, 0, sizeof(fake));
        fake.type = SAPP_EVENTTYPE_KEY_UP;

        int mods[] = {
            SAPP_KEYCODE_LEFT_ALT,   SAPP_KEYCODE_RIGHT_ALT,
            SAPP_KEYCODE_LEFT_CONTROL,  SAPP_KEYCODE_RIGHT_CONTROL,
            SAPP_KEYCODE_LEFT_SHIFT, SAPP_KEYCODE_RIGHT_SHIFT,
        };

        for (int i = 0; i < (int)(sizeof(mods)/sizeof(mods[0])); i++) {
            fake.key_code = mods[i];
            enqueue_event(&fake);
        }

        lua_pushstring(L, "unfocused");
        return 1;
    }

    case SAPP_EVENTTYPE_KEY_DOWN:
        lua_pushstring(L, "keypressed");
        lua_pushstring(L, key_name(buf, e.key_code));
        return 2;

    case SAPP_EVENTTYPE_KEY_UP:
        lua_pushstring(L, "keyreleased");
        lua_pushstring(L, key_name(buf, e.key_code));
        return 2;

    case SAPP_EVENTTYPE_CHAR: {
        // MELHORADO: Suporte adequado para UTF-8
        char utf8_buffer[5] = {0}; // 4 bytes para UTF-8 + null terminator
        int bytes_written = utf8_encode(e.char_code, utf8_buffer);
        
        if (bytes_written > 0) {
            lua_pushstring(L, "textinput");
            lua_pushlstring(L, utf8_buffer, bytes_written);
            return 2;
        }
        return 0; // Se falhou a codificação, não retorna evento
    }

    case SAPP_EVENTTYPE_MOUSE_DOWN: {
        int clicks = get_click_count(&e);
        lua_pushstring(L, "mousepressed");
        lua_pushstring(L, button_name(e.mouse_button));
        lua_pushnumber(L, e.mouse_x);
        lua_pushnumber(L, e.mouse_y);
        lua_pushnumber(L, clicks);
        return 5;
    }

    case SAPP_EVENTTYPE_MOUSE_UP:
        lua_pushstring(L, "mousereleased");
        lua_pushstring(L, button_name(e.mouse_button));
        lua_pushnumber(L, e.mouse_x);
        lua_pushnumber(L, e.mouse_y);
        return 4;

    case SAPP_EVENTTYPE_MOUSE_MOVE:
        lua_pushstring(L, "mousemoved");
        lua_pushnumber(L, e.mouse_x);
        lua_pushnumber(L, e.mouse_y);
        lua_pushnumber(L, e.mouse_dx);
        lua_pushnumber(L, e.mouse_dy);
        return 5;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        lua_pushstring(L, "mousewheel");
        lua_pushnumber(L, e.scroll_y);
        return 2;

    default:
        return 0;
    }
}

static int f_sleep(lua_State* L) {
    double n = luaL_checknumber(L, 1); // seconds
#ifdef _WIN32
    Sleep((DWORD)(n * 1000)); // Sleep uses milliseconds
#else
    struct timespec ts;
    ts.tv_sec = (time_t)n;
    ts.tv_nsec = (long)((n - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif
    return 0;
}

static int f_absolute_path(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    char* res = realpath(path, NULL);
    if (!res) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushstring(L, res);
    free(res);
    return 1;
}

static int f_get_clipboard(lua_State* L) {
    const char* text = sapp_get_clipboard_string();
    if (!text) { return 0; }
    lua_pushstring(L, text);
    return 1;
}

static int f_set_clipboard(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    sapp_set_clipboard_string(text);
    return 0;
}

// @TODO(ellora): Support utf8 instead?
static int f_fuzzy_match(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    const char* ptn = luaL_checkstring(L, 2);
    int score = 0;
    int run = 0;
    while (*str && *ptn) {
        while (*str == ' ') { str++; }
        while (*ptn == ' ') { ptn++; }
        if (tolower(*str) == tolower(*ptn)) {
            score += run * 10 - (*str != *ptn);
            run++;
            ptn++;
        } else {
            score -= 10;
            run = 0;
        }
        str++;
    }
    if (*ptn) { return 0; }
    lua_pushnumber(L, score - (int)strlen(str));
    return 1;
}

static int f_exec(lua_State* L) {
    size_t len;
    const char* cmd = luaL_checklstring(L, 1, &len);
    char* buf = malloc(len + 32);
    if (!buf) { luaL_error(L, "buffer allocation failed"); }
#if _WIN32
    sprintf(buf, "cmd /c \"%s\"", cmd);
    WinExec(buf, SW_HIDE);
#else
    sprintf(buf, "%s &", cmd);
    int res = system(buf);
    (void)res;
#endif
    free(buf);
    return 0;
}

static const luaL_Reg lib[] = {
    {"poll_event", f_poll_event},
    {"set_cursor", f_set_cursor},
    {"set_window_title", f_set_window_title},
    {"set_window_mode", f_set_window_mode},
    {"window_has_focus", f_window_has_focus},
    {"show_confirm_dialog", f_show_confirm_dialog},
    {"chdir", f_chdir},
    {"list_dir", f_list_dir},
    {"absolute_path", f_absolute_path},
    {"get_file_info", f_get_file_info},
    {"get_clipboard", f_get_clipboard},
    {"set_clipboard", f_set_clipboard},
    {"get_time", f_get_time},
    {"sleep", f_sleep},
    {"exec", f_exec},
    {"fuzzy_match", f_fuzzy_match},
    {NULL, NULL}
};

int luaopen_system(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}