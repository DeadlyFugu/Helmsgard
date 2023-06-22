#include "common.h"
#include <stdarg.h>

int Math_lerp(int a, int b, float f) {
    return a + (int) ((b - a) * f);
}

int Math_sanitizeAngle(int a) {
    if (a >= 0) {
        return a % 360;
    } else {
        return 360 - (-a % 360);
    }
}

int Math_angleTo(int fromX, int fromY, int toX, int toY) {
    int a = Math_atan2(-(toY - fromY), toX - fromX);
    return a < 0 ? a + 360 : a;
}

int Math_lengthSquared(int x, int y) {
    return x * x + y * y;
}

int Math_angleLengthX(int angle, int length) {
    return (int) (Math_cos(angle) * length);
}

int Math_angleLengthY(int angle, int length) {
    return (int) (-Math_sin(angle) * length);
}

float Math_clampf(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value);
}

void Math_normalizeXY(float* x, float* y) {
    float length = sqrtf(*x * *x + *y * *y); // that's a lot of asterisks
    if (length == 0) return;
    *x /= length;
    *y /= length;
}

int String_parseInt(const char* string, int def) {
    if (!string) return def;
    char* end;
    int result = strtol(string, &end, 10);
    if (*end) {
        Log_warn("invalid integer '%s'", string);
        return def;
    }
    return result;
}

bool String_parseBool(const char* string, bool def) {
    if (!string || string[0] == 0) return def;

    return strcmp(string, "yes") == 0 || strcmp(string, "true") == 0;
}

float String_parseFloat(const char* string, float def) {
    if (!string) return def;
    char* end;
    float result = strtof(string, &end);
    if (*end) {
        Log_warn("invalid float '%s'", string);
        return def;
    }
    return result;
}

void String_parseInt2(const char* string, int* a, int* b) {
    int values[2];
    if (String_parseIntArray(string, values, 2) == 2) {
        *a = values[0];
        *b = values[1];
    }
}

int String_parseIntArray(const char* string, int* array, int max) {
    return String_parseIntArrayExt(string, array, max, atoi);
}

int String_parseIntArrayExt(const char* string, int* array, int max,
        int (*fnConvert)(const char*)) {
    if (!string) return 0;

    char buf[512];
    strncpy(buf, string, 512);

    char* token = strtok(buf, ",");
    int i;
    for (i = 0; i < max && token; i++) {
        array[i] = fnConvert(String_trimLocal(token));
        token = strtok(NULL, ",");
    }
    
    return i;
}


int String_parseEnum(const char* string, const char* values, int def) {
    if (!string) return def;

    char buf[256];
    strncpy(buf, values, 256);

    int i = 0;
    char* token = strtok(buf, ";");
    while (token) {
        if (strcmp(string, token) == 0) return i;
        i++;
        token = strtok(NULL, ";");
    }

    Log_warn("unknown enum value %s, expected %s", string, values);

    return def;
}

char* String_trimLocal(char* string) {
    char* begin = string;
    while (*begin == ' ' || *begin == '\t' || *begin == '\n') begin++;
    char* end = begin;
    while (*end) end++;
    end--;
    while (*end == ' ' || *end == '\t' || *end == '\n') end--;
    end++;
    *end = 0;
    return begin;
}

// max is amount of parts, len is width of each part buffer
int String_split(const char* string, char delim, char** parts, int max, int len) {
    char delimStr[2];
    delimStr[0] = delim;
    delimStr[1] = 0;

    const char* begin = string;
    int i;
    for (i = 0; i < max && *begin; i++) {
        const char* end = begin;

        if (i < max - 1) {
            while (*end && *end != delim) end++;
            int width = end - begin;
            strncpy(parts[i], begin, SDL_min(len, width));
        } else {
            strncpy(parts[i], begin, len);
        }

        begin = *end ? end + 1 : end;
    }

    return i;
}

void String_parseIntDirs(const char* string, int* array,
        int (*fnConvert)(const char*), int def) {
    int count = String_parseIntArrayExt(string, array, 4, fnConvert);
    if (count == 1) {
        array[1] = array[0];
        array[2] = array[0];
        array[3] = array[0];
    } else if (count == 2) {
        array[2] = array[1];
        array[3] = array[1];
        array[1] = array[0];
    } else if (count != 4) {
        if (string) Log_warn("Expected 1, 2, or 4 elements in '%s'", string);
        for (int i = 0; i < 4; i++) array[i] = def;
    }
}

void Log_error(const char* format, ...) {
    va_list args;
    char buf[512];
    va_start(args, format);
    vsnprintf(buf, 512, format, args);
    va_end(args);

    fprintf(stderr, "\x1b[31merror: %s\x1b[0m\n", buf);
}

void Log_warn(const char* format, ...) {
    if (Config_logLevel < 1) return;
    va_list args;
    char buf[512];
    va_start(args, format);
    vsnprintf(buf, 512, format, args);
    va_end(args);

    fprintf(stderr, "\x1b[33mwarn: %s\x1b[0m\n", buf);
}

void Log_info(const char* format, ...) {
    if (Config_logLevel < 2) return;
    va_list args;
    char buf[512];
    va_start(args, format);
    vsnprintf(buf, 512, format, args);
    va_end(args);

    fprintf(stderr, "info: %s\x1b[0m\n", buf);
}

void Log_debug(const char* format, ...) {
    if (Config_logLevel < 3) return;
    va_list args;
    char buf[512];
    va_start(args, format);
    vsnprintf(buf, 512, format, args);
    va_end(args);

    fprintf(stderr, "\x1b[32mdebug: %s\x1b[0m\n", buf);
}
