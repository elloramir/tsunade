// That file is basically the original renderer.h file
// from RXI, but without SDL stuff. Also the typedef tricky
// is not necessary here.

#ifndef RENDERER_H
#define RENDERER_H

#include <sokol_gfx.h>
#include <sokol_gl.h>
#include <fontstash.h>

typedef struct { uint8_t b, g, r, a; } RenColor;
typedef struct { int x, y, width, height; } RenRect;

typedef struct RenImage
{
    sg_image image;
    int width;
    int height;
} RenImage;

typedef struct RenFont
{
    FONScontext* fons_context;
    int font_id;
    float size;
    int tab_width;
} RenFont;


void ren_init(void);
void ren_shutdown(void);
void ren_begin_frame(void);
void ren_end_frame(void);

void ren_set_clip_rect(RenRect rect);
void ren_get_size(int *x, int *y);

RenImage* ren_new_image(int width, int height);
void ren_free_image(RenImage *image);

RenFont* ren_load_font(const char *filename, float size);
void ren_free_font(RenFont *font);
void ren_set_font_tab_width(RenFont *font, int n);
int ren_get_font_tab_width(RenFont *font);
int ren_get_font_width(RenFont *font, const char *text);
int ren_get_font_height(RenFont *font);

void ren_draw_rect(RenRect rect, RenColor color);
void ren_draw_image(RenImage *image, RenRect *sub, int x, int y, RenColor color);
int ren_draw_text(RenFont *font, const char *text, int x, int y, RenColor color);

#endif