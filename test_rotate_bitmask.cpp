/*
 * test_rotate_bitmask.cpp
 *
 * Cross-validates bitmask rotateX/rotateZ against the mt1 cell-based approach.
 *
 * Uses the EXACT same packed bitmask format as box.hpp:
 *   uint8_t mask[12]  — 24 cells * 4 bits = 96 bits, two cells per byte
 *
 * cellState(x,y,z): (mask[i>>1] >> ((i&1)<<2)) & 0xf
 * setCellState(x,y,z,s): clear 4 bits, write s
 *
 * Two test modes:
 *   1. Exhaustive: all 2^12 = 4096 states of empty/full/direction
 *   2. Random: random states with heavy direction coverage
 *
 * Build: g++ -O2 -o test_rotate_bitmask test_rotate_bitmask.cpp -std=c++17
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <set>

// ── Constants (match box.hpp) ────────────────────────────────────────────────
static const int BOX_W = 4, BOX_H = 3, BOX_D = 2;
static const int BOX_CELLS = BOX_W * BOX_H * BOX_D; // 24

// ── Direction encoding ───────────────────────────────────────────────────────
static const int dirVec[14][3] = {
    {0,0,0},   // 0: empty
    {0,0,0},   // 1: full
    {1,0,1},   // 2: +x +z
    {1,0,-1},  // 3: +x -z
    {-1,0,1},  // 4: -x +z
    {-1,0,-1}, // 5: -x -z
    {0,1,1},   // 6: +y +z
    {0,-1,-1}, // 7: -y -z
    {1,1,0},   // 8: +x +y
    {1,-1,0},  // 9: +x -y
    {-1,1,0},  // 10: -x +y
    {-1,-1,0}, // 11: -x -y
    {0,1,-1},  // 12: +y -z
    {0,-1,1},  // 13: -y +z
};

static const uint8_t oppositeCode[14] = {
    0, 1, 5, 4, 3, 2, 7, 6, 11, 10, 9, 8, 13, 12
};

// directionSwapZ and directionSwapX from box.hpp
static const uint8_t directionSwapZ[16] = {
    0, 1, 4, 5, 2, 3, 13, 12, 11, 10, 9, 8, 7, 6
};

static const uint8_t directionSwapX[16] = {
    0, 1, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12
};

static uint8_t encodeDir(int x, int y, int z) {
    for (int i = 2; i <= 13; i++)
        if (dirVec[i][0]==x && dirVec[i][1]==y && dirVec[i][2]==z) return i;
    return 0;
}

static void decodeDir(uint8_t code, int &x, int &y, int &z) {
    if (code >= 2 && code <= 13) { x = dirVec[code][0]; y = dirVec[code][1]; z = dirVec[code][2]; }
    else { x = y = z = 0; }
}

// ── Cell-based box (reference, from mt1 branch) ──────────────────────────────
struct Cell {
    int state; // 0=empty, 1=full, 2=half
    int dir[3];
    Cell() : state(0), dir{0,0,0} {}
    bool operator==(const Cell &o) const {
        return state==o.state && dir[0]==o.dir[0] && dir[1]==o.dir[1] && dir[2]==o.dir[2];
    }
};

struct CellBox {
    Cell content[BOX_W][BOX_H][BOX_D];
    CellBox() { reset(); }
    void reset() { for(int x=0;x<BOX_W;x++) for(int y=0;y<BOX_H;y++) for(int z=0;z<BOX_D;z++) content[x][y][z]=Cell(); }

    Cell& getCube(int x, int y, int z) { return content[x][y][z]; }
    const Cell& getCube(int x, int y, int z) const { return content[x][y][z]; }

    bool operator==(const CellBox &o) const {
        for(int x=0;x<BOX_W;x++) for(int y=0;y<BOX_H;y++) for(int z=0;z<BOX_D;z++)
            if(!(content[x][y][z]==o.content[x][y][z])) return false;
        return true;
    }
    bool operator!=(const CellBox &o) const { return !(*this == o); }

    // Reference rotateZ: pairwise swap with direction transform via codes.
    // Iterates all cells, processes each pair once via ordering check.
    void rotateZ_ref() {
        for(int z=0;z<BOX_D;z++)
            for(int y=0;y<BOX_H;y++)
                for(int x=0;x<BOX_W;x++) {
                    int x2 = BOX_W-1-x, y2 = BOX_H-1-y;
                    // Only process if this is the "first" in the pair
                    if(x > x2 || (x == x2 && y > y2)) continue;
                    // Encode current direction codes
                    uint8_t code1 = 0, code2 = 0;
                    if(content[x][y][z].state == 2)
                        code1 = encodeDir(content[x][y][z].dir[0],
                                          content[x][y][z].dir[1],
                                          content[x][y][z].dir[2]);
                    if(content[x2][y2][z].state == 2)
                        code2 = encodeDir(content[x2][y2][z].dir[0],
                                          content[x2][y2][z].dir[1],
                                          content[x2][y2][z].dir[2]);
                    // Transform using direction swap tables
                    uint8_t ncode1 = (code1 >= 2 && code1 <= 13) ? directionSwapZ[code1] : 0;
                    uint8_t ncode2 = (code2 >= 2 && code2 <= 13) ? directionSwapZ[code2] : 0;
                    // Write transformed cells
                    if(ncode1) {
                        content[x2][y2][z].state = 2;
                        decodeDir(ncode1, content[x2][y2][z].dir[0],
                                  content[x2][y2][z].dir[1],
                                  content[x2][y2][z].dir[2]);
                    } else {
                        content[x2][y2][z].state = (code1 == 1) ? 1 : 0;
                        content[x2][y2][z].dir[0] = content[x2][y2][z].dir[1] =
                        content[x2][y2][z].dir[2] = 0;
                    }
                    if(ncode2) {
                        content[x][y][z].state = 2;
                        decodeDir(ncode2, content[x][y][z].dir[0],
                                  content[x][y][z].dir[1],
                                  content[x][y][z].dir[2]);
                    } else {
                        content[x][y][z].state = (code2 == 1) ? 1 : 0;
                        content[x][y][z].dir[0] = content[x][y][z].dir[1] =
                        content[x][y][z].dir[2] = 0;
                    }
                }
    }

    // Reference rotateX: pairwise swap with direction transform via codes.
    void rotateX_ref() {
        for(int z=0;z<BOX_D;z++)
            for(int y=0;y<BOX_H;y++)
                for(int x=0;x<BOX_W;x++) {
                    int y2 = BOX_H-1-y, z2 = BOX_D-1-z;
                    // Only process if this is the "first" in the pair
                    if(y > y2 || (y == y2 && z > z2)) continue;
                    // Encode current direction codes
                    uint8_t code1 = 0, code2 = 0;
                    if(content[x][y][z].state == 2)
                        code1 = encodeDir(content[x][y][z].dir[0],
                                          content[x][y][z].dir[1],
                                          content[x][y][z].dir[2]);
                    if(content[x][y2][z2].state == 2)
                        code2 = encodeDir(content[x][y2][z2].dir[0],
                                          content[x][y2][z2].dir[1],
                                          content[x][y2][z2].dir[2]);
                    // Transform using direction swap tables
                    uint8_t ncode1 = (code1 >= 2 && code1 <= 13) ? directionSwapX[code1] : 0;
                    uint8_t ncode2 = (code2 >= 2 && code2 <= 13) ? directionSwapX[code2] : 0;
                    // Write transformed cells
                    if(ncode1) {
                        content[x][y2][z2].state = 2;
                        decodeDir(ncode1, content[x][y2][z2].dir[0],
                                  content[x][y2][z2].dir[1],
                                  content[x][y2][z2].dir[2]);
                    } else {
                        content[x][y2][z2].state = (code1 == 1) ? 1 : 0;
                        content[x][y2][z2].dir[0] = content[x][y2][z2].dir[1] =
                        content[x][y2][z2].dir[2] = 0;
                    }
                    if(ncode2) {
                        content[x][y][z].state = 2;
                        decodeDir(ncode2, content[x][y][z].dir[0],
                                  content[x][y][z].dir[1],
                                  content[x][y][z].dir[2]);
                    } else {
                        content[x][y][z].state = (code2 == 1) ? 1 : 0;
                        content[x][y][z].dir[0] = content[x][y][z].dir[1] =
                        content[x][y][z].dir[2] = 0;
                    }
                }
    }

    void dump(const char* label="") const {
        if(label[0]) printf("  %s:\n", label);
        for(int z=0;z<BOX_D;z++) {
            printf("    z=%d:", z);
            for(int y=0;y<BOX_H;y++) {
                for(int x=0;x<BOX_W;x++) {
                    const Cell &c = content[x][y][z];
                    if(c.state==1) printf(" X");
                    else if(c.state==2) printf(" +%d%d%d", c.dir[0], c.dir[1], c.dir[2]);
                    else printf(" .");
                }
            }
            printf("\n");
        }
    }
};

// ── Bitmask box (exact copy of box.hpp format) ───────────────────────────────
struct BitBox {
    uint8_t mask[12];

    BitBox() { reset(); }
    void reset() { std::memset(mask, 0, sizeof(mask)); }

    inline uint8_t cellState(int x, int y, int z) const {
        int i = x + BOX_W*y + BOX_W*BOX_H*z;
        return (mask[i>>1] >> ((i&1)<<2)) & 0xf;
    }

    inline void setCellState(int x, int y, int z, uint8_t s) {
        int i = x + BOX_W*y + BOX_W*BOX_H*z;
        mask[i>>1] &= (uint8_t)~(0xf << ((i&1)<<2));
        mask[i>>1] |= (uint8_t)(s << ((i&1)<<2));
    }

    bool operator==(const BitBox &o) const {
        return std::memcmp(mask, o.mask, sizeof(mask)) == 0;
    }

    // ── Bitmask rotateZ (fixed: pairwise swap, process each pair once) ─
    void rotateZ_bm() {
        for (int z = 0; z < BOX_D; z++)
            for (int y = 0; y < BOX_H; y++)
                for (int x = 0; x < BOX_W; x++) {
                    int x2 = BOX_W - 1 - x;
                    int y2 = BOX_H - 1 - y;
                    // Only process if this is the "first" in the pair
                    if (x > x2 || (x == x2 && y > y2)) continue;
                    uint8_t code1 = cellState(x, y, z);
                    uint8_t code2 = cellState(x2, y2, z);
                    // Swap: use full code as direction swap lookup index (codes 2-13).
                    if (code1 >= 2 && code1 <= 13)
                        setCellState(x2, y2, z, directionSwapZ[code1]);
                    else
                        setCellState(x2, y2, z, code1);
                    if (code2 >= 2 && code2 <= 13)
                        setCellState(x, y, z, directionSwapZ[code2]);
                    else
                        setCellState(x, y, z, code2);
                }
    }

    // ── Bitmask rotateX (fixed: pairwise swap, process each pair once) ─
    void rotateX_bm() {
        for (int x = 0; x < BOX_W; x++)
            for (int y = 0; y < BOX_H; y++)
                for (int z = 0; z < BOX_D; z++) {
                    int y2 = BOX_H - 1 - y;
                    int z2 = BOX_D - 1 - z;
                    // Only process if this is the "first" in the pair
                    if (y > y2 || (y == y2 && z > z2)) continue;
                    uint8_t code1 = cellState(x, y, z);
                    uint8_t code2 = cellState(x, y2, z2);
                    // Swap: use full code as direction swap lookup index (codes 2-13).
                    if (code1 >= 2 && code1 <= 13)
                        setCellState(x, y2, z2, directionSwapX[code1]);
                    else
                        setCellState(x, y, z, code1);
                    if (code2 >= 2 && code2 <= 13)
                        setCellState(x, y, z, directionSwapX[code2]);
                    else
                        setCellState(x, y, z, code2);
                }
    }

    // Convert to CellBox for comparison
    CellBox toCellBox() const {
        CellBox cb; cb.reset();
        for(int x=0;x<BOX_W;x++) for(int y=0;y<BOX_H;y++) for(int z=0;z<BOX_D;z++) {
            uint8_t code = cellState(x, y, z);
            Cell &c = cb.content[x][y][z];
            if(code==1) c.state = 1;
            else if(code>=2 && code<=13) { c.state = 2; decodeDir(code, c.dir[0], c.dir[1], c.dir[2]); }
        }
        return cb;
    }

    // Set from CellBox
    void fromCellBox(const CellBox &cb) {
        reset();
        for(int x=0;x<BOX_W;x++) for(int y=0;y<BOX_H;y++) for(int z=0;z<BOX_D;z++) {
            const Cell &c = cb.content[x][y][z];
            if(c.state==1) setCellState(x,y,z, 1);
            else if(c.state==2) setCellState(x,y,z, encodeDir(c.dir[0], c.dir[1], c.dir[2]));
        }
    }

    void dump(const char* label="") const {
        if(label[0]) printf("  %s:\n", label);
        for(int z=0;z<BOX_D;z++) {
            printf("    z=%d:", z);
            for(int y=0;y<BOX_H;y++) {
                for(int x=0;x<BOX_W;x++) {
                    uint8_t code = cellState(x,y,z);
                    if(code==1) printf(" X");
                    else if(code>=2 && code<=13) { int dx,dy,dz; decodeDir(code,dx,dy,dz); printf(" +%d%d%d",dx,dy,dz); }
                    else printf(" .");
                }
            }
            printf("\n");
        }
    }
};

// ── Globals ──────────────────────────────────────────────────────────────────
static int testCount = 0, passCount = 0;
static std::set<std::string> failedCells; // deduplicate repeated failures

#define CHECK(cond, msg) do { \
    testCount++; \
    if (cond) { passCount++; } \
    else { printf("  FAIL #%d: %s\n", testCount, (msg)); } \
} while(0)

#define CHECK_FMT(cond, fmt, ...) do { \
    testCount++; \
    char _msg[512]; snprintf(_msg, sizeof(_msg), fmt, ##__VA_ARGS__); \
    if (cond) { passCount++; } \
    else { printf("  FAIL #%d: %s\n", testCount, _msg); } \
} while(0)

// ── Test: direction swap table correctness ───────────────────────────────────
static void test_direction_swap_tables() {
    printf("\n=== Test: direction swap table correctness ===\n");

    // Verify directionSwapZ: for each direction, negate x,y and check code matches
    for (int d = 2; d <= 13; d++) {
        int dx, dy, dz;
        decodeDir(d, dx, dy, dz);
        int nx = -dx, ny = -dy, nz = dz;
        int expected = encodeDir(nx, ny, nz);
        int actual = directionSwapZ[d];
        CHECK_FMT(expected == actual,
            "directionSwapZ[%d]: expected %d (from (%d,%d,%d)->(%d,%d,%d)), got %d",
            d, expected, dx, dy, dz, nx, ny, nz, actual);
    }

    // Verify directionSwapX: for each direction, negate y,z and check code matches
    for (int d = 2; d <= 13; d++) {
        int dx, dy, dz;
        decodeDir(d, dx, dy, dz);
        int nx = dx, ny = -dy, nz = -dz;
        int expected = encodeDir(nx, ny, nz);
        int actual = directionSwapX[d];
        CHECK_FMT(expected == actual,
            "directionSwapX[%d]: expected %d (from (%d,%d,%d)->(%d,%d,%d)), got %d",
            d, expected, dx, dy, dz, nx, ny, nz, actual);
    }

    // Verify 4x composition = identity
    for (int d = 2; d <= 13; d++) {
        uint8_t r = d;
        r = directionSwapZ[r]; r = directionSwapZ[r]; r = directionSwapZ[r]; r = directionSwapZ[r];
        CHECK(r == (uint8_t)d, "directionSwapZ 4x = identity");
        r = d;
        r = directionSwapX[r]; r = directionSwapX[r]; r = directionSwapX[r]; r = directionSwapX[r];
        CHECK(r == (uint8_t)d, "directionSwapX 4x = identity");
    }
}

// ── Test: cell->bitmask->cell round-trip ─────────────────────────────────────
static void test_roundtrip() {
    printf("\n=== Test: cell->bitmask->cell round-trip ===\n");

    // Test all possible single-cell states
    for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++) {
        // Empty
        { CellBox cb; cb.reset(); BitBox bb; bb.fromCellBox(cb); CellBox cb2 = bb.toCellBox();
          CHECK(cb==cb2, "empty round-trip"); }
        // Full
        { CellBox cb; cb.reset(); cb.content[x][y][z].state=1; BitBox bb; bb.fromCellBox(cb); CellBox cb2 = bb.toCellBox();
          CHECK(cb==cb2, "full round-trip"); }
        // All 12 directions
        for (int d=2;d<=13;d++) {
            CellBox cb; cb.reset(); cb.content[x][y][z].state=2;
            decodeDir(d, cb.content[x][y][z].dir[0], cb.content[x][y][z].dir[1], cb.content[x][y][z].dir[2]);
            BitBox bb; bb.fromCellBox(cb); CellBox cb2 = bb.toCellBox();
            char msg[128]; snprintf(msg, sizeof(msg), "dir %d round-trip at [%d,%d,%d]", d, x, y, z);
            CHECK(cb==cb2, msg);
        }
    }
}

// ── Test: exhaustive single-cell states ──────────────────────────────────────
static void test_single_cell() {
    printf("\n=== Test: single-cell exhaustive (cell->bitmask->rotate->compare) ===\n");

    for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++) {
        // Full cell
        {
            CellBox cb; cb.reset(); cb.content[x][y][z].state=1;
            BitBox bb; bb.fromCellBox(cb);

            BitBox bbZ = bb; CellBox cbZ_ref = cb;
            bbZ.rotateZ_bm(); cbZ_ref.rotateZ_ref();
            CellBox cbZ_bm = bbZ.toCellBox();
            if (cbZ_ref != cbZ_bm) {
                failedCells.insert("rotateZ_full");
            }
            CHECK(cbZ_ref == cbZ_bm, "rotateZ full cell");

            BitBox bbX = bb; CellBox cbX_ref = cb;
            bbX.rotateX_bm(); cbX_ref.rotateX_ref();
            CellBox cbX_bm = bbX.toCellBox();
            if (cbX_ref != cbX_bm) {
                failedCells.insert("rotateX_full");
            }
            CHECK(cbX_ref == cbX_bm, "rotateX full cell");
        }

        // Each direction
        for (int d=2;d<=13;d++) {
            CellBox cb; cb.reset();
            cb.content[x][y][z].state = 2;
            decodeDir(d, cb.content[x][y][z].dir[0], cb.content[x][y][z].dir[1], cb.content[x][y][z].dir[2]);

            BitBox bb; bb.fromCellBox(cb);

            // rotateZ
            {
                BitBox bbZ = bb; CellBox cbZ_ref = cb;
                bbZ.rotateZ_bm(); cbZ_ref.rotateZ_ref();
                CellBox cbZ_bm = bbZ.toCellBox();
                if (cbZ_ref != cbZ_bm) {
                    char msg[256];
                    int dx,dy,dz; decodeDir(d,dx,dy,dz);
                    snprintf(msg, sizeof(msg), "rotateZ dir[%d](%d,%d,%d) at [%d,%d,%d]", d, dx, dy, dz, x, y, z);
                    failedCells.insert(msg);
                    printf("\n  --- rotateZ MISMATCH ---\n");
                    printf("  Input:\n"); cb.dump("    in");
                    printf("  Expected (ref):\n"); cbZ_ref.dump("    ref");
                    printf("  Got (bm):\n"); cbZ_bm.dump("    bm");
                    // Show direction swap lookup
                    int nx = BOX_W-1-x, ny = BOX_H-1-y, nz = z;
                    int vx = -dx, vy = -dy, vz = dz;
                    int expectedCode = encodeDir(vx, vy, vz);
                    int actualCode = directionSwapZ[d];
                    printf("  dirSwapZ[%d]=%d, expected=%d (from (%d,%d,%d)->(%d,%d,%d))\n",
                        d, directionSwapZ[d], expectedCode, dx,dy,dz, vx,vy,vz);
                }
                CHECK_FMT(cbZ_ref == cbZ_bm,
                    "rotateZ dir[%d] at [%d,%d,%d]", d, x, y, z);
            }

            // rotateX
            {
                BitBox bbX = bb; CellBox cbX_ref = cb;
                bbX.rotateX_bm(); cbX_ref.rotateX_ref();
                CellBox cbX_bm = bbX.toCellBox();
                if (cbX_ref != cbX_bm) {
                    char msg[256];
                    int dx,dy,dz; decodeDir(d,dx,dy,dz);
                    snprintf(msg, sizeof(msg), "rotateX dir[%d](%d,%d,%d) at [%d,%d,%d]", d, dx, dy, dz, x, y, z);
                    failedCells.insert(msg);
                    printf("\n  --- rotateX MISMATCH ---\n");
                    printf("  Input:\n"); cb.dump("    in");
                    printf("  Expected (ref):\n"); cbX_ref.dump("    ref");
                    printf("  Got (bm):\n"); cbX_bm.dump("    bm");
                    int ny = BOX_H-1-y, nz = BOX_D-1-z;
                    int vx = dx, vy = -dy, vz = -dz;
                    int expectedCode = encodeDir(vx, vy, vz);
                    int actualCode = directionSwapX[d];
                    printf("  dirSwapX[%d]=%d, expected=%d (from (%d,%d,%d)->(%d,%d,%d))\n",
                        d, directionSwapX[d], expectedCode, dx,dy,dz, vx,vy,vz);
                }
                CHECK_FMT(cbX_ref == cbX_bm,
                    "rotateX dir[%d] at [%d,%d,%d]", d, x, y, z);
            }
        }
    }
}

// ── Test: multi-cell random states ───────────────────────────────────────────
static void test_random(int n) {
    printf("\n=== Test: random states (%d) ===\n", n);
    srand(42);

    for (int t = 0; t < n; t++) {
        CellBox cb; cb.reset();
        BitBox bb; bb.reset();

        // Build random state with good direction coverage
        for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++) {
            int r = rand() % 14;
            if (r == 1) {
                cb.content[x][y][z].state = 1;
            } else if (r >= 2 && r <= 13) {
                cb.content[x][y][z].state = 2;
                decodeDir(r, cb.content[x][y][z].dir[0], cb.content[x][y][z].dir[1], cb.content[x][y][z].dir[2]);
            }
            // else: empty
        }

        bb.fromCellBox(cb);

        // Verify initial state matches
        CellBox check = bb.toCellBox();
        if (check != cb) {
            printf("  FAIL #%d: initial state mismatch at test %d\n", ++testCount, t);
            continue;
        }
        testCount++; passCount++;

        // rotateZ
        {
            BitBox bbZ = bb; CellBox cbZ_ref = cb;
            bbZ.rotateZ_bm(); cbZ_ref.rotateZ_ref();
            CellBox cbZ_bm = bbZ.toCellBox();
            if (cbZ_ref != cbZ_bm) {
                failedCells.insert("random_Z");
                printf("\n  --- RANDOM rotateZ MISMATCH test %d ---\n", t);
                cbZ_ref.dump("    ref");
                cbZ_bm.dump("    bm");
            }
            CHECK(cbZ_ref == cbZ_bm, "rotateZ random");
        }

        // rotateX
        {
            BitBox bbX = bb; CellBox cbX_ref = cb;
            bbX.rotateX_bm(); cbX_ref.rotateX_ref();
            CellBox cbX_bm = bbX.toCellBox();
            if (cbX_ref != cbX_bm) {
                failedCells.insert("random_X");
                printf("\n  --- RANDOM rotateX MISMATCH test %d ---\n", t);
                cbX_ref.dump("    ref");
                cbX_bm.dump("    bm");
            }
            CHECK(cbX_ref == cbX_bm, "rotateX random");
        }
    }
}

// ── Test: 4x rotation identity ───────────────────────────────────────────────
static void test_identity() {
    printf("\n=== Test: 4x rotation identity ===\n");
    srand(99);

    for (int t = 0; t < 500; t++) {
        BitBox bb; bb.reset();
        CellBox cb; cb.reset();

        for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++) {
            int r = rand() % 14;
            if (r == 1) { bb.setCellState(x,y,z,1); cb.content[x][y][z].state=1; }
            else if (r >= 2 && r <= 13) {
                bb.setCellState(x,y,z,r);
                cb.content[x][y][z].state=2;
                decodeDir(r, cb.content[x][y][z].dir[0], cb.content[x][y][z].dir[1], cb.content[x][y][z].dir[2]);
            }
        }

        // rotateZ x4
        {
            BitBox bbZ = bb; CellBox cbZ = cb;
            bbZ.rotateZ_bm(); bbZ.rotateZ_bm(); bbZ.rotateZ_bm(); bbZ.rotateZ_bm();
            cbZ.rotateZ_ref(); cbZ.rotateZ_ref(); cbZ.rotateZ_ref(); cbZ.rotateZ_ref();
            CHECK(bbZ == bb, "rotateZ x4 identity (bitmask)");
            CHECK(cbZ == cb, "rotateZ x4 identity (ref)");
        }

        // rotateX x4
        {
            BitBox bbX = bb; CellBox cbX = cb;
            bbX.rotateX_bm(); bbX.rotateX_bm(); bbX.rotateX_bm(); bbX.rotateX_bm();
            cbX.rotateX_ref(); cbX.rotateX_ref(); cbX.rotateX_ref(); cbX.rotateX_ref();
            CHECK(bbX == bb, "rotateX x4 identity (bitmask)");
            CHECK(cbX == cb, "rotateX x4 identity (ref)");
        }
    }
}

// ── Test: direction swap lookup detailed report ──────────────────────────────
static void test_lookup_report() {
    printf("\n=== direction swap lookup report ===\n");
    printf("  code  vector   negated    expected   lookup   match\n");
    for (int d = 2; d <= 13; d++) {
        int dx, dy, dz;
        decodeDir(d, dx, dy, dz);

        // Z: negate x,y
        int vxZ = -dx, vyZ = -dy, vzZ = dz;
        int expectedZ = encodeDir(vxZ, vyZ, vzZ);
        int actualZ = directionSwapZ[d];

        // X: negate y,z
        int vxX = dx, vyX = -dy, vzX = -dz;
        int expectedX = encodeDir(vxX, vyX, vzX);
        int actualX = directionSwapX[d];

        printf("  %2d  (%2d,%2d,%2d)  Z:(%2d,%2d,%2d)  exp=%2d  got=%2d  %s\n",
            d, dx,dy,dz, vxZ,vyZ,vzZ, expectedZ, actualZ, expectedZ==actualZ?"OK":"FAIL");
        printf("                          X:(%2d,%2d,%2d)  exp=%2d  got=%2d  %s\n",
            vxX,vyX,vzX, expectedX, actualX, expectedX==actualX?"OK":"FAIL");
    }
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main() {
    printf("=== Bitmask Rotate Cross-Validation ===\n");
    printf("Box: %dx%dx%d, bitmask: 12 bytes (4 bits/cell)\n\n", BOX_W, BOX_H, BOX_D);

    test_lookup_report();
    test_direction_swap_tables();
    test_roundtrip();
    test_single_cell();
    test_random(2000);
    test_identity();

    printf("\n=== Results: %d/%d passed (%zu unique failures) ===\n",
           passCount, testCount, failedCells.size());

    if (!failedCells.empty()) {
        printf("\nFailed tests:\n");
        for (const auto &f : failedCells) printf("  - %s\n", f.c_str());
    }

    return (passCount == testCount) ? 0 : 1;
}
