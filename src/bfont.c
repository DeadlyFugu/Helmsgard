#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_BFONTS 8
#define MAX_GLYPHS 128

// ===== [[ Local Types ]] =====

typedef struct {
    char name[NAME_LENGTH];
    bool forceUpper;
    int lookup[128];
    int codepoints[MAX_GLYPHS];
    int xoffs[MAX_GLYPHS];
    int yoffs[MAX_GLYPHS];
    int widths[MAX_GLYPHS];
    int descender[MAX_GLYPHS];
    int glyphCount;
    int height;
    int charGap;
    int lineSpacing;
    int spaceWidth;
} BFont;

// ===== [[ Declarations ]] =====

static int _ustack(const char* code, int* stack, int limit);
static int _findGlyph(BFont* bfont, int codepoint);

// ===== [[ Static Data ]] =====

static BFont _bfonts[MAX_GLYPHS];
static int _bfontCount;
static SDL_Texture* _texBFonts;

// ===== [[ Implementations ]] =====

void BFont_load(void) {
    Ini_readAsset("bfonts.ini");

    for (int i = 0; Ini_getSectionName(i); i++) {
        const char* name = Ini_getSectionName(i);
        if (!name[0]) continue;
        if (_bfontCount == MAX_BFONTS) {
            Log_error("Max bfonts exceeded");
            Ini_clear();
            return;
        }

        BFont* bfont = &_bfonts[_bfontCount++];
        strncpy(bfont->name, name, NAME_LENGTH);

        bfont->glyphCount = _ustack(
            Ini_get(name, "codepoints"),
            bfont->codepoints, MAX_GLYPHS);

        int count = _ustack(
            Ini_get(name, "xoffs"),
            bfont->xoffs, MAX_GLYPHS
        );
        if (count != bfont->glyphCount) {
            Log_warn("xoffs size in bfont doesnt match");
            bfont->glyphCount = SDL_min(bfont->glyphCount, count);
        }

        // for (int i = 0; i < count; i++) {
        //     Log_debug("stack[%d] = %d", i, bfont->xoffs[i]);
        // }

        count = _ustack(
            Ini_get(name, "yoffs"),
            bfont->yoffs, MAX_GLYPHS
        );
        if (count != bfont->glyphCount) {
            Log_warn("yoffs size in bfont doesnt match");
            bfont->glyphCount = SDL_min(bfont->glyphCount, count);
        }

        count = _ustack(
            Ini_get(name, "widths"),
            bfont->widths, MAX_GLYPHS
        );
        if (count != bfont->glyphCount) {
            Log_warn("widths size in bfont doesnt match");
            bfont->glyphCount = SDL_min(bfont->glyphCount, count);
        }

        const char* descenders = Ini_get(name, "descenders");
        for (int i = 0; i < bfont->glyphCount; i++) {
            bfont->descender[i] = (descenders &&
                strchr(descenders, bfont->codepoints[i]) != 0) ? 3 : 0;
        }

        // build lookup table
        for (int i = 0; i < 128; i++) {
            bfont->lookup[i] = -1;
        }
        for (int i = 0; i < bfont->glyphCount; i++) {
            int codepoint = bfont->codepoints[i];
            if (codepoint >= 0 && codepoint < 128) {
                bfont->lookup[codepoint] = i;
            }
        }

        bfont->forceUpper =
            String_parseBool(Ini_get(name, "forceUpper"), false);
        bfont->height = String_parseInt(Ini_get(name, "height"), 8);
        bfont->charGap = String_parseInt(Ini_get(name, "charGap"), 1);
        bfont->lineSpacing = String_parseInt(Ini_get(name, "lineSpacing"), 3);
        bfont->spaceWidth = String_parseInt(Ini_get(name, "spaceWidth"), 3);
    }

    Ini_clear();

    _texBFonts = Loading_loadTexture("assets/images/bfonts.bmp");
}

BFontID BFont_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < _bfontCount; i++) {
        if (strcmp(_bfonts[i].name, name) == 0) return i;
    }
    Log_warn("Cannot find bfont %s", name);
    return -1;
}

void BFont_drawText(BFontID font, int x, int y, const char* string, ...) {
    if (font < 0 || font > _bfontCount) return;
    BFont* bfont = &_bfonts[font];

    char buf[256];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 256, string, lst);
    va_end(lst);

    int cx = x;
    int cy = y;
    bool needGap = false;
    for (int i = 0; buf[i]; i++) {
        char c = buf[i];
        if (c == ' ') {
            cx += bfont->spaceWidth;
            needGap = false;
        } else if (c == '\n') {
            cx = x;
            cy += bfont->height + bfont->lineSpacing;
            needGap = false;
        } else {
            if (bfont->forceUpper && c >= 'a' && c <= 'z') {
                c += ('A' - 'a');
            }
            int glyph = _findGlyph(bfont, c);
            if (glyph == -1) continue;
            if (needGap) cx += bfont->charGap;

            SDL_Rect src = {
                bfont->xoffs[glyph], bfont->yoffs[glyph],
                bfont->widths[glyph], bfont->height
            };
            SDL_Rect dst = {
                cx, cy + bfont->descender[glyph],
                bfont->widths[glyph], bfont->height
            };
            Draw_translatePoint(&dst.x, &dst.y);
            SDL_RenderCopy(Main_renderer, _texBFonts, &src, &dst);
            cx += bfont->widths[glyph];
            needGap = true;
        }
    }
}

void BFont_drawTextExt(BFontID font, int x, int y,
        int maxWidth, float halign,
        const char* string, ...
) {
    if (font < 0 || font > _bfontCount) return;
    BFont* bfont = &_bfonts[font];

    char buf[512];
    va_list lst;
    va_start(lst, string);
    vsnprintf(buf, 512, string, lst);
    va_end(lst);

    int rowStart = 0;
    int cx = x;
    int cy = y;
    int curr = 0;
    bool needGap = false;
    while (buf[curr]) {
        // determine largest row we can fit
        int rowEnd = rowStart; // max candidate row end
        int rowWidth = 0;
        int rowWidthCandidate = 0;
        needGap = false;
        while (rowWidth <= maxWidth || rowEnd == rowStart) {
            char c = buf[curr];
            if (c == 0) {
                rowEnd = curr;
                rowWidthCandidate = rowWidth;
                break;
            } else if (c == ' ') {
                rowEnd = curr;
                rowWidthCandidate = rowWidth;
                rowWidth += bfont->spaceWidth;
                needGap = false;
            } else if (c == '\n') {
                rowEnd = curr;
                rowWidthCandidate = rowWidth;
                needGap = false;
                break;
            } else {
                if (bfont->forceUpper && c >= 'a' && c <= 'z') {
                    c += ('A' - 'a');
                }
                int glyph = _findGlyph(bfont, c);
                if (glyph == -1) continue;
                if (needGap) rowWidth += bfont->charGap;

                rowWidth += bfont->widths[glyph];
                needGap = true;
            }
            curr++;
        }

        // draw row
        cx = x + (maxWidth - rowWidthCandidate) * halign;
        needGap = false;
        for (int i = rowStart; i < rowEnd; i++) {
            char c = buf[i];
            if (c == ' ') {
                cx += bfont->spaceWidth;
                needGap = false;
            } else {
                if (bfont->forceUpper && c >= 'a' && c <= 'z') {
                    c += ('A' - 'a');
                }
                int glyph = _findGlyph(bfont, c);
                if (glyph == -1) continue;
                if (needGap) cx += bfont->charGap;

                SDL_Rect src = {
                    bfont->xoffs[glyph], bfont->yoffs[glyph],
                    bfont->widths[glyph], bfont->height
                };
                SDL_Rect dst = {
                    cx, cy + bfont->descender[glyph],
                    bfont->widths[glyph], bfont->height
                };
                Draw_translatePoint(&dst.x, &dst.y);
                SDL_RenderCopy(Main_renderer, _texBFonts, &src, &dst);
                cx += bfont->widths[glyph];
                needGap = true;
            }
        }

        // reset to beginning of next row
        curr = rowEnd;
        while (buf[curr] == ' ' || buf[curr] == '\n') curr++;
        rowStart = curr;
        cy += bfont->height + bfont->lineSpacing;
    }
}

static int _ustack(const char* code, int* stack, int limit) {
    if (!code) return 0;
    const char* curr = code;
    int size = 0;
    int marker = 0;
    while (*curr) {
        switch (*curr) {
            case '.': {
                curr++;
                if (!curr) goto bad_eol;
                if (size == limit) goto overflow;
                stack[size++] = *curr;
            } break;
            case '~': {
                if (size < 2) goto underflow;
                size -= 2;
                int begin = stack[size];
                int end = stack[size + 1];
                while (begin <= end) {
                    if (size == limit) goto overflow;
                    stack[size++] = begin++;
                }
            } break;
            case '-': {
                if (size < 1) goto underflow;
                stack[size - 1] = -stack[size - 1];
            } break;
            case '^': {
                if (size < 1) goto underflow;
                if (size == limit) goto overflow;
                int value = stack[size - 1];
                stack[size++] = value;
            } break;
            case '/': {
                if (size < 2) goto underflow;
                size -= 2;
                int value = stack[size];
                int times = stack[size + 1];
                for (int i = 0; i < times; i++) {
                    if (size == limit) goto overflow;
                    stack[size++] = value;
                }
            } break;
            case ' ': break;
            case ',': break;
            case '|': marker = size; break;
            case '+': {
                if (size < 1) goto underflow;
                size--;
                int value = stack[size];
                for (int i = marker; i < size; i++) {
                    stack[i] += value;
                }
            } break;
            case '*': {
                if (size < 1) goto underflow;
                size--;
                int value = stack[size];
                for (int i = marker; i < size; i++) {
                    stack[i] *= value;
                }
            } break;
            default: {
                if (*curr >= '0' && *curr <= '9') {
                    int value = 0;
                    while (*curr >= '0' && *curr <= '9') {
                        value = value * 10 + (*curr - '0');
                        curr++;
                    }
                    curr--;
                    if (size == limit) goto overflow;
                    stack[size++] = value;
                } else {
                    Log_warn("ustack ignoring '%c'", *curr);
                }
            }
        }
        curr++;
    }

    return size;

    bad_eol:
    Log_error("unexpected eol in ustack code");
    return size;

    overflow:
    Log_error("stack overflow in ustack code");
    return size;

    underflow:
    Log_error("stack underflow in ustack code");
    return size;
}

static int _findGlyph(BFont* bfont, int codepoint) {
    int result = -1;
    if (codepoint >= 0 && codepoint < 128) {
        result = bfont->lookup[codepoint];
    } else {
        for (int i = 0; i < bfont->glyphCount; i++) {
            if (bfont->codepoints[i] == codepoint) {
                return i;
            }
        }
    }

    if (result == -1) {
        if (bfont->lookup[0] != -1) {
            return bfont->lookup[0];
        }
    }
    return result;
}
