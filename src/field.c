#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_LINES 1024
#define MAX_VERTICES 1024

// ===== [[ Local Types ]] =====

typedef struct {
    int x1, y1;
    int x2, y2;
    int midX, midY;
    int normalAngle;
} FieldLine;

typedef struct {
    int x, y;
    int normalAngle;
} FieldVertex;

// ===== [[ Declarations ]] =====

static bool _areAnglesReflex(int a, int b);
static int _dot(int x1, int y1, int x2, int y2);

// ===== [[ Static Data ]] =====

static FieldLine _lines[MAX_LINES];
static FieldVertex _vertices[MAX_VERTICES];
static int _lineCount;
static int _vertexCount;

// ===== [[ Implementations ]] =====

void Field_clear(void) {
    _lineCount = 0;
    _vertexCount = 0;
}

void Field_addRect(int x, int y, int w, int h) {
    FieldPoint points[] = {
        { x, y },
        { x + w, y },
        { x + w, y + h },
        { x, y + h}
    };

    Field_addPolygon(4, points, false);
}

void Field_addPolygon(int count, FieldPoint* points, bool ccw) {
    // DEBUG
    // Log_debug("Begin polygon (%d vertices)", count);
    // for (int i = 0; i < count; i++) {
    //     Log_debug(" Vertex[%d] = (%d, %d)", i, points[i].x, points[i].y);
    // }

    // check enough space for vertices/lines
    if (_vertexCount + count > countof(_vertices) ||
            _lineCount + count > countof(_lines)) {
        printf("WARN ignoring field polygon (exceeded limit)\n");
        return;
    }

    // add lines between points
    int baseLine = _lineCount;
    for (int i = 0; i < count; i++) {
        int next = (i + 1) % count;

        // add line
        FieldLine* line = &_lines[baseLine + i];
        *line = (FieldLine) {
            .x1 = points[i].x,
            .y1 = points[i].y,
            .x2 = points[next].x,
            .y2 = points[next].y
        };

        // precalculate line properties
        line->midX = line->x1 + (line->x2 - line->x1) / 2;
        line->midY = line->y1 + (line->y2 - line->y1) / 2;
        int tangentAngle = Math_angleTo(line->x1, line->y1, line->x2, line->y2);
        line->normalAngle = Math_sanitizeAngle(tangentAngle + (ccw ? -90 : 90));
    }

    // add vertices for each corner
    int baseVtx = _vertexCount;
    for (int i = 0; i < count; i++) {
        int next = (i + 1) % count;

        // add vertex
        // note: we add the next vertex, as we need access to the line
        //       before to determine vertex normal
        int prevN = _lines[baseLine + i].normalAngle;
        int nextN = _lines[baseLine + next].normalAngle;
        int middleNormal = (Math_sanitizeAngle(prevN + 180 - nextN) - 180) / 2 + nextN;
        _vertices[baseVtx + i] = (FieldVertex) {
            .x = points[next].x,
            .y = points[next].y,
            .normalAngle = Math_sanitizeAngle(middleNormal)
        };
    }

    _vertexCount += count;
    _lineCount += count;
}

FieldNearest Field_findNearest(int x, int y) {
    int nearestX = 0, nearestY = 0;
    int nearestNormal = 0;
    int nearestDistSq = INT32_MAX;

    // find nearest point on nearest line
    for (int i = 0; i < _lineCount; i++) {
        // find nearest point on line
        FieldLine* line = &_lines[i];
        int vx = line->x2 - line->x1;
        int vy = line->y2 - line->y1;
        int ux = line->x1 - x;
        int uy = line->y1 - y;
        float t = _dot(-vx, -vy, ux, uy) / (float) Math_lengthSquared(vx, vy);
        if (t < 0 || t > 1) continue; // nearest point outside of line seg
        int px = Math_lerp(line->x1, line->x2, t);
        int py = Math_lerp(line->y1, line->y2, t);

        // update if this point is closer
        int distSq = Math_lengthSquared(x - px, y - py);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestX = px;
            nearestY = py;
            nearestNormal = line->normalAngle;
        }
    }

    // find any vertices nearer than that
    for (int i = 0; i < _vertexCount; i++) {
        FieldVertex* vertex = &_vertices[i];
        // todo: does this have overflow issues?
        int distSq = Math_lengthSquared(x - vertex->x, y - vertex->y);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestX = vertex->x;
            nearestY = vertex->y;
            nearestNormal = vertex->normalAngle;
        }
    }

    int angleFromNearest = Math_angleTo(nearestX, nearestY, x, y);
    return (FieldNearest) {
        .x = nearestX,
        .y = nearestY,
        .angle = nearestNormal,
        .distanceSquared = nearestDistSq,
        .isInside = _areAnglesReflex(angleFromNearest, nearestNormal)
    };
}

void Field_drawDebug(void) {
    for (int i = 0; i < _lineCount; i++) {
        // draw lines
        FieldLine* line = &_lines[i];
        Draw_setColor(Color_maroon);
        Draw_line(line->x1 / 16, line->y1 / 16,
                line->x2 / 16, line->y2 / 16);

        // draw line normals
        int nx = line->midX + Math_angleLengthX(line->normalAngle, 160);
        int ny = line->midY + Math_angleLengthY(line->normalAngle, 160);
        Draw_setColor(Color_olive);
        Draw_line(line->midX / 16, line->midY / 16, nx / 16, ny / 16);
    }

    for (int i = 0; i < _vertexCount; i++) {
        // draw vertex normals
        FieldVertex* vertex = &_vertices[i];
        int nx = vertex->x + Math_angleLengthX(vertex->normalAngle, 160);
        int ny = vertex->y + Math_angleLengthY(vertex->normalAngle, 160);
        Draw_setColor(Color_green);
        Draw_line(vertex->x / 16, vertex->y / 16, nx / 16, ny / 16);
    }
}

void Field_drawDebugCollision(int x, int y) {
    FieldNearest nearest = Field_findNearest(x, y);

    if (nearest.isInside) {
        Draw_setColor(Color_red);
    } else {
        Draw_setColor(Color_blue);
    }
    Draw_line(nearest.x / 16, nearest.y / 16, x / 16, y / 16);

    int nx = nearest.x + Math_angleLengthX(nearest.angle, 160);
    int ny = nearest.y + Math_angleLengthY(nearest.angle, 160);
    Draw_setColor(Color_lime);
    Draw_line(nearest.x / 16, nearest.y / 16, nx / 16, ny / 16);
}

static bool _areAnglesReflex(int a, int b) {
    while (b < a - 180) b += 360;
    while (b > a + 180) b -= 360;
    return b < a - 90 || b > a + 90;
}

static int _dot(int x1, int y1, int x2, int y2) {
    return x1 * x2 + y1 * y2;
}
