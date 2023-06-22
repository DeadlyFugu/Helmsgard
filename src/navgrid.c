#include "common.h"

// ===== [[ Defines ]] =====

#define MAX_NAVGRID_WIDTH 128
#define MAX_NAVGRID_HEIGHT 128

// ===== [[ Local Types ]] =====

typedef struct {
    int x, y;
} Vector2i;

// ===== [[ Declarations ]] =====

// ===== [[ Static Data ]] =====

static int _gridWidth;
static int _gridHeight;
static bool _gridSolid[MAX_NAVGRID_WIDTH][MAX_NAVGRID_HEIGHT];

// ===== [[ Implementations ]] =====

void NavGrid_set(int width, int height, bool* data) {
    if (width > MAX_NAVGRID_WIDTH || height > MAX_NAVGRID_HEIGHT) {
        Log_warn("data too large for navgrid");
        return;
    }

    memset(_gridSolid, 0, MAX_NAVGRID_WIDTH * MAX_NAVGRID_HEIGHT * sizeof(bool));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            _gridSolid[x][y] = data[y * width + x];
        }
    }

    _gridWidth = width;
    _gridHeight = height;
}

int NavGrid_findPath(int fromX, int fromY, int toX, int toY,
    int maxLength, int* pathXs, int* pathYs
) {
    // do nothing if either point is out of navgrid bounds
    if (fromX < 0 || fromY < 0 || toX < 0 || toY < 0 ||
        fromX > _gridWidth || fromY > _gridHeight ||
        toX > _gridWidth || toY > _gridHeight
    ) {
        return 0;
    }

    bool* visited = calloc(_gridWidth * _gridHeight, sizeof(Vector2i));
    Vector2i* nexts = calloc(_gridWidth * _gridHeight, sizeof(Vector2i));
    // n.b. search set is bounded by grid size
    // (actually bounded lower)
    Vector2i* searchSet = calloc(_gridWidth * _gridHeight, sizeof(Vector2i));
    int searchSetCount = 0;

    // start search set at dest, work back to start
    searchSet[0] = (Vector2i) { toX, toY };
    searchSetCount++;

    bool found = false;
    while (searchSetCount > 0) {
        // find closest in search set
        int closest = 0;
        int closestDist = abs(searchSet[0].x - fromX) + abs(searchSet[0].y - fromY);
        for (int i = 1; i < searchSetCount; i++) {
            int dist = abs(searchSet[i].x - fromX) + abs(searchSet[i].y - fromY);
            if (dist < closestDist) {
                closest = i;
                closestDist = dist;
            }
        }

        // remove closest from search set
        Vector2i current = searchSet[closest];
        searchSet[closest] = searchSet[searchSetCount - 1];
        searchSetCount--;

        // if its source then we found a path
        if (current.x == fromX && current.y == fromY) {
            found = true;
            break;
        }

        // otherwise add all neighbours to search
        for (int nx = current.x-1; nx <= current.x+1; nx++) {
            for (int ny = current.y-1; ny <= current.y+1; ny++) {
                if (nx == current.x && ny == current.y) continue;
                if (nx < 0 || ny < 0 || nx >= _gridWidth ||
                    ny >= _gridHeight) continue;
                int index = ny * _gridWidth + nx;
                if (visited[index]) continue;
                if (_gridSolid[nx][ny]) continue;
                visited[index] = true;
                nexts[index] = current;
                searchSet[searchSetCount++] = (Vector2i) { nx, ny };
            }
        }
    }

    free(visited);
    free(searchSet);

    // abort if no path found
    if (!found) {
        free(nexts);
        return -1;
    }

    // rebuild path
    Vector2i current = { fromX, fromY };
    int length = 0;
    while (current.x != toX || current.y != toY) {
        if (length < maxLength) {
            pathXs[length] = current.x;
            pathYs[length] = current.y;
        }
        length++;
        int index = current.y * _gridWidth + current.x;
        current = nexts[index];
    }

    free(nexts);
    return length;
}
