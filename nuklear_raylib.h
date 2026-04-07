#ifndef NK_RAYLIB_H
#define NK_RAYLIB_H

#include "nuklear.h"
#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

// 核心宏（写死，绝对不会未定义）
#define nk_rgba(r,g,b,a) ((struct nk_color){(r), (g), (b), (a)})
#define nk_rgb(r,g,b)    nk_rgba((r), (g), (b), 255)
#define nk_rect(x,y,w,h) ((struct nk_rect){(x), (y), (w), (h)})
#define nk_vec2(x,y)     ((struct nk_vec2){(x), (y)})

// 窗口标志
#define NK_WINDOW_NO_SCROLLBAR  NK_NO_SCROLLBAR
#define NK_WINDOW_BORDER        NK_BORDER
#define NK_WINDOW_MOVABLE       NK_MOVABLE
#define NK_WINDOW_SCALABLE      NK_SCALABLE
#define NK_WINDOW_CLOSABLE      NK_CLOSABLE

// 兼容宏
#define nk_panel_begin(ctx, name, rect, flags) nk_begin((ctx), (name), (rect), (flags))
#define nk_panel_end(ctx)                       nk_end((ctx))
#define nk_text_heading(ctx, text)              nk_text((ctx), (text), nk_strlen(text), NK_TEXT_LEFT)
#define nk_text_label(ctx, text)                nk_text((ctx), (text), nk_strlen(text), NK_TEXT_LEFT)

// 填充高亮宏
#define nk_widget_fill_color(ctx, rect, col, hover_col) \
    do { \
        nk_fill_rect((ctx), (rect), 0, (col)); \
        if (nk_input_is_mouse_hovering_rect(&(ctx)->input, (rect))) \
            nk_fill_rect((ctx), (rect), 0, (hover_col)); \
    } while(0)

// 图表宏
#define NK_CHART_COLUMN 0
#define NK_CHART_LINE   1
#define NK_CHART_AREA   2

// 函数声明
NK_API struct nk_context* nk_raylib_init(float font_size);
NK_API void nk_raylib_shutdown(struct nk_context *ctx);
NK_API void nk_raylib_input(struct nk_context *ctx);
NK_API void nk_raylib_render(struct nk_context *ctx, struct nk_color clear_color);
NK_API void nk_raylib_set_font(struct nk_context *ctx, Font font);
NK_API void nk_style_set_blue(struct nk_context *ctx);
NK_API void nk_chart_floating(struct nk_context *ctx, int type, const float *data, int count, float min, float max);

// 实现部分（仅当定义NK_RAYLIB_IMPLEMENTATION时编译，避免重复定义）
#ifdef NK_RAYLIB_IMPLEMENTATION

static Font g_nk_font = {0};

struct nk_context* nk_raylib_init(float font_size) {
    struct nk_context *ctx = nk_init_default(NULL, NULL);
    if (ctx) nk_style_set_blue(ctx);
    return ctx;
}

void nk_raylib_shutdown(struct nk_context *ctx) {
    if (ctx) nk_free(ctx);
    if (g_nk_font.texture.id) UnloadFont(g_nk_font);
}

void nk_raylib_input(struct nk_context *ctx) {
    if (!ctx) return;
    Vector2 mouse = GetMousePosition();
    nk_input_begin(ctx);
    nk_input_motion(&ctx->input, (int)mouse.x, (int)mouse.y);
    nk_input_button(&ctx->input, NK_BUTTON_LEFT, (int)mouse.x, (int)mouse.y, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    nk_input_button(&ctx->input, NK_BUTTON_RIGHT, (int)mouse.x, (int)mouse.y, IsMouseButtonDown(MOUSE_BUTTON_RIGHT));
    nk_input_end(ctx);
}

void nk_raylib_render(struct nk_context *ctx, struct nk_color clear_color) {
    if (!ctx) return;
    ClearBackground((Color){clear_color.r, clear_color.g, clear_color.b, clear_color.a});
    nk_render(ctx);
}

void nk_raylib_set_font(struct nk_context *ctx, Font font) {
    if (!ctx || !font.texture.id) return;
    g_nk_font = font;
    SetFont(font);
}

void nk_style_set_blue(struct nk_context *ctx) {
    if (!ctx) return;
    struct nk_color bg = nk_rgba(245, 248, 252, 255);
    struct nk_color panel_bg = nk_rgba(255, 255, 255, 255);
    struct nk_color primary = nk_rgba(22, 119, 255, 255);
    struct nk_color hover = nk_rgba(64, 158, 255, 255);
    struct nk_color active = nk_rgba(9, 109, 217, 255);

    ctx->style.window.fixed_background = bg;
    ctx->style.window.background = panel_bg;
    ctx->style.window.border_color = nk_rgba(230, 230, 230, 255);
    ctx->style.window.header.normal = primary;
    ctx->style.window.header.text = nk_rgb(255, 255, 255);

    ctx->style.button.normal = primary;
    ctx->style.button.hover = hover;
    ctx->style.button.active = active;
    ctx->style.button.text_normal = nk_rgb(255, 255, 255);
    ctx->style.button.text_hover = nk_rgb(255, 255, 255);
    ctx->style.button.text_active = nk_rgb(255, 255, 255);

    ctx->style.text.color = nk_rgb(0, 0, 0);
    ctx->style.panel.color = panel_bg;
}

void nk_chart_floating(struct nk_context *ctx, int type, const float *data, int count, float min, float max) {
    if (!ctx || !data || count <= 0) return;
    if (type == NK_CHART_LINE) nk_chart_line(ctx, data, count, min, max);
    else if (type == NK_CHART_COLUMN) nk_chart_column(ctx, data, count, min, max);
    else if (type == NK_CHART_AREA) nk_chart_area(ctx, data, count, min, max);
}

#endif // NK_RAYLIB_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif // NK_RAYLIB_H