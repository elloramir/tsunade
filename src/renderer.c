#include "renderer.h"

#define SOKOL_GFX_IMPL
#include <sokol_gfx.h>

#define SOKOL_GL_IMPL
#include <sokol_gl.h>

#define FONTSTASH_IMPLEMENTATION
#include <fontstash.h>
#define SOKOL_FONTSTASH_IMPL
#include <sokol_fontstash.h>

#include <sokol_app.h>
#include <sokol_log.h>
#include <sokol_glue.h>

#include <stdlib.h>
#include <stdio.h>

static struct
{
    FONScontext* fs;
    sg_pass_action pass_action;
    sg_sampler sampler;
    sgl_pipeline pip;
}
state;

void ren_init(void) {
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    sgl_setup(&(sgl_desc_t){0});

    sfons_desc_t fons_desc = {
        .width = 512,
        .height = 512,
    };
    state.fs = sfons_create(&fons_desc);

    sg_sampler_desc smp_desc = {
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
    };
    state.sampler = sg_make_sampler(&smp_desc);

    state.pass_action = (sg_pass_action) {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0.0f, 0.0f, 0.0f, 1.0f} }
    };

    // Enable blend (alpha colors)
    state.pip = sgl_make_pipeline(&(sg_pipeline_desc){
        .colors[0] = {
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_ONE,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_alpha = SG_BLENDOP_ADD
            }
        },
        .label = "pipeline-with-blending"
    });
}

void ren_shutdown(void) {
    sfons_destroy(state.fs);
    sg_destroy_sampler(state.sampler);
    sgl_shutdown();
    sg_shutdown();
}

void ren_begin_frame(void) {
    int w = sapp_width();
    int h = sapp_height();
    sgl_viewport(0, 0, w, h, true);
    sgl_defaults();
    sgl_matrix_mode_projection();
    sgl_ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);
    sgl_push_pipeline();
    sgl_load_pipeline(state.pip);
}

void ren_end_frame(void) {
    sgl_pop_pipeline();
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    sgl_draw();
    sfons_flush(state.fs);
    sg_end_pass();
    sg_commit();
}

void ren_set_clip_rect(RenRect rect) {
    sgl_scissor_rect(rect.x, rect.y, rect.width, rect.height, true);
}

void ren_get_size(int *x, int *y) {
    *x = sapp_width();
    *y = sapp_height();
}

RenImage* ren_new_image(int width, int height) {
    // @todo(ellora): The implementation for creating a new image is commented out.
    // It should allocate a RenImage, set its dimensions, and create a sokol_gfx image.
    // Return NULL on failure.
    return NULL;
}

void ren_free_image(RenImage *image) {
    // @todo(ellora): The implementation for freeing an image is commented out.
    // It should destroy the sokol_gfx image and free the RenImage struct.
    // It should handle NULL pointers gracefully.
}

RenFont* ren_load_font(const char *filename, float size) {
    RenFont* font = malloc(sizeof(RenFont));
    if (!font) return NULL;
    font->fons_context = state.fs;

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        free(font);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    unsigned char* data = malloc(file_size);
    if (!data) {
        fclose(fp);
        free(font);
        return NULL;
    }
    fread(data, 1, file_size, fp);
    fclose(fp);

    font->font_id = fonsAddFontMem(state.fs, "default", data, (int)file_size, 1);
    if (font->font_id == FONS_INVALID) {
        free(font);
        return NULL;
    }
    font->size = size;
    font->tab_width = 4;
    return font;
}

void ren_free_font(RenFont *font) {
    free(font);
}

void ren_set_font_tab_width(RenFont *font, int n) {
    font->tab_width = n;
}

int ren_get_font_tab_width(RenFont *font) {
    return font->tab_width;
}

int ren_get_font_width(RenFont *font, const char *text) {
    fonsSetFont(state.fs, font->font_id);
    fonsSetSize(state.fs, font->size);
    return (int)fonsTextBounds(state.fs, 0, 0, text, NULL, NULL);
}

int ren_get_font_height(RenFont *font) {
    fonsSetFont(state.fs, font->font_id);
    fonsSetSize(state.fs, font->size);
    float asc, desc, lh;
    fonsVertMetrics(state.fs, &asc, &desc, &lh);
    return (int)lh;
}

void ren_draw_rect(RenRect rect, RenColor color) {
    sgl_begin_quads();
    sgl_c4b(color.r, color.g, color.b, color.a);
    sgl_v2f((float)rect.x, (float)rect.y);
    sgl_v2f((float)rect.x + rect.width, (float)rect.y);
    sgl_v2f((float)rect.x + rect.width, (float)rect.y + rect.height);
    sgl_v2f((float)rect.x, (float)rect.y + rect.height);
    sgl_end();
}

void ren_draw_image(RenImage *image, RenRect *sub, int x, int y, RenColor color) {
    // @todo(ellora): The implementation for drawing an image is commented out.
    // It should handle texture mapping, sub-rectangles, and color tinting using sokol-gl.
}

int ren_draw_text(RenFont *font, const char *text, int x, int y, RenColor color) {
    fonsSetFont(state.fs, font->font_id);
    fonsSetSize(state.fs, font->size);
    fonsSetColor(state.fs, sfons_rgba(color.r, color.g, color.b, color.a));
    return (int)fonsDrawText(state.fs, (float)x, (float)y + font->size/1.25, text, NULL);
}