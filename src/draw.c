#include "SDL_render.h"
#include "common.h"

// ===== [[ Static Data ]] =====

const SDL_Color Color_aqua = { 0, 255, 255, 255 };
const SDL_Color Color_black = { 0, 0, 0, 255 };
const SDL_Color Color_blue = { 0, 0, 255, 255 };
const SDL_Color Color_dkgray = { 64, 64, 64, 255 };
const SDL_Color Color_fuchsia = { 255, 0, 255, 255 };
const SDL_Color Color_gray = { 128, 128, 128, 255 };
const SDL_Color Color_green = { 0, 128, 0, 255 };
const SDL_Color Color_lime = { 0, 255, 0, 255 };
const SDL_Color Color_ltgray = { 192, 192, 192, 255 };
const SDL_Color Color_maroon = { 128, 0, 0, 255 };
const SDL_Color Color_navy = { 0, 0, 128, 255 };
const SDL_Color Color_olive = { 128, 128, 0, 255 };
const SDL_Color Color_orange = { 255, 160, 64, 255 };
const SDL_Color Color_purple = { 128, 0, 128, 255 };
const SDL_Color Color_red = { 255, 0, 0, 255 };
const SDL_Color Color_silver = { 192, 192, 192, 255 };
const SDL_Color Color_teal = { 0, 128, 128, 255 };
const SDL_Color Color_white = { 255, 255, 255, 255 };
const SDL_Color Color_yellow = { 255, 255, 0, 255 };

//static FC_Font* _font;
static SDL_Color _color = { 255, 255, 255, 255 };
static int _tx, _ty;

// ===== [[ Implementations ]] =====

/*void Draw_setFont(FC_Font* font) {
    _font = font;
}*/

void Draw_setColor(SDL_Color color) {
    _color = color;
    SDL_SetRenderDrawColor(Main_renderer,
            _color.r, _color.g, _color.b, _color.a);
}

void Draw_setAlpha(float alpha) {
    _color.a = 255 * alpha;
    SDL_SetRenderDrawColor(Main_renderer,
            _color.r, _color.g, _color.b, _color.a);
}

void Draw_setTranslate(int x, int y) {
    _tx = x;
    _ty = y;
}

void Draw_setClip(int x, int y, int w, int h) {
    SDL_Rect clip = { x, y, w, h };
    SDL_RenderSetClipRect(Main_renderer, &clip);
}

void Draw_clearClip(void) {
    SDL_RenderSetClipRect(Main_renderer, NULL);
}

void Draw_translatePoint(int* x, int* y) {
    *x += _tx;
    *y += _ty;
}

void Draw_point(int x, int y) {
    SDL_RenderDrawPoint(Main_renderer, x + _tx, y + _ty);
}

void Draw_line(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(Main_renderer,
        x1 + _tx, y1 + _ty, x2 + _tx, y2 + _ty);
}

void Draw_rect(int x, int y, int w, int h) {
    SDL_Rect rect = { x + _tx, y + _ty, w, h };
    SDL_RenderFillRect(Main_renderer, &rect);
}

void Draw_rectOutline(int x, int y, int w, int h) {
    SDL_Rect rect = { x + _tx, y + _ty, w, h };
    SDL_RenderDrawRect(Main_renderer, &rect);
}

void Draw_circle(int x, int y, int radius) {
    int px = x + (int) (Math_cos(340) * radius);
    int py = y + (int) (-Math_sin(340) * radius);
    for (int i = 0; i < 18; i++) {
        int nx = x + (int) (Math_cos(i * 20) * radius);
        int ny = y + (int) (-Math_sin(i * 20) * radius);
        Draw_line(px, py, nx, ny);
        px = nx; py = ny;
    }
}

void Draw_text(int x, int y, const char* string, ...) {
    char buf[256];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 256, string, lst);
    va_end(lst);

    //FC_DrawColor(_font, Main_renderer, x + _tx, y + _ty, _color, buf);
}

void Draw_textAligned(int x, int y, float halign, float valign,
        const char* string, ...) {
    char buf[256];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 256, string, lst);
    va_end(lst);

    /*int w = FC_GetWidth(_font, buf);
    int h = FC_GetHeight(_font, buf);
    int bx = x + _tx - halign * w;
    int by = y + _ty - valign * h;
    FC_DrawColor(_font, Main_renderer, bx, by, _color, buf);*/
}

int Draw_getTextWidth(const char* string, ...) {
    char buf[256];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 256, string, lst);
    va_end(lst);

    //return FC_GetWidth(_font, buf);
	return strlen(buf) * 6;
}

int Draw_getTextHeight(const char* string, ...) {
    char buf[256];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 256, string, lst);
    va_end(lst);

    //return FC_GetHeight(_font, buf);
	return 10;
}
