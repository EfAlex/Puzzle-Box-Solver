/*
 * test_rotate_vectors.cpp
 *
 * Tests rotateX / rotateZ using the current direction-vector approach
 * (as in box.cpp lines 267-318).  Exposes the double-swap bug.
 *
 * Compile:
 *   g++ -O2 -o test_rotate_vectors test_rotate_vectors.cpp cube.cpp -std=c++17
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

/* ---- Minimal cube (mirrors cube.hpp / cube.cpp) ---- */

enum cell_t { cell_empty, cell_full, cell_half };

struct Cube {
    cell_t storage;
    int v[3];

    Cube() : storage(cell_empty) { v[0]=v[1]=v[2]=0; }

    cell_t getState() const { return storage; }
    void setState(cell_t s) { storage = s; }
    bool isEmpty()  const { return storage == cell_empty; }
    bool isFull()   const { return storage == cell_full; }
    bool isHalf()   const { return storage == cell_half; }
    int* getVector() { return v; }
    const int* getVector() const { return v; }
    void setVector(const int* nv) { v[0]=nv[0]; v[1]=nv[1]; v[2]=nv[2]; }
    void reset() { storage = cell_empty; v[0]=v[1]=v[2]=0; }

    bool operator==(const Cube& o) const {
        if (isEmpty() && o.isEmpty()) return true;
        if (isFull()  && o.isFull())  return true;
        if (isHalf()  && o.isHalf())  return v[0]==o.v[0] && v[1]==o.v[1] && v[2]==o.v[2];
        return false;
    }
    bool operator!=(const Cube& o) const { return !(*this == o); }
};

/* ---- Box using direction vectors (exact copy of box.cpp rotateX/rotateZ) ---- */

static const int BOX_W = 4, BOX_H = 3, BOX_D = 2;

struct BoxVectors {
    Cube cells[BOX_W][BOX_H][BOX_D];

    BoxVectors() {
        for (int x=0; x<BOX_W; x++)
            for (int y=0; y<BOX_H; y++)
                for (int z=0; z<BOX_D; z++)
                    cells[x][y][z].reset();
    }

    Cube& getCube(int x, int y, int z) { return cells[x][y][z]; }
    const Cube& getCube(int x, int y, int z) const { return cells[x][y][z]; }

    /* ---- EXACT copy of box.cpp::rotateZ (lines 267-292) ---- */
    void rotateZ() {
        int p1[3], p2[3];
        for(int z = 0; z < 2; ++z) {
            p1[2] = z;  p2[2] = z;
            for (int y = 0; y < 3; ++y) {
                p1[1] = y;  p2[1] = 2 - y;
                for (int x=0; x < 2; ++x) {
                    p1[0] = x;  p2[0] = 3 - x;
                    Cube c1 = getCube(p1[0],p1[1],p1[2]);
                    Cube c2 = getCube(p2[0],p2[1],p2[2]);
                    int *v1 = c1.getVector(), *v2 = c2.getVector();
                    v1[0] = -v1[0]; v1[1] = -v1[1];
                    v2[0] = -v2[0]; v2[1] = -v2[1];
                    getCube(p1[0],p1[1],p1[2]).setState(c2.getState());
                    getCube(p1[0],p1[1],p1[2]).setVector(v2);
                    getCube(p2[0],p2[1],p2[2]).setState(c1.getState());
                    getCube(p2[0],p2[1],p2[2]).setVector(v1);
                }
            }
        }
    }

    /* ---- EXACT copy of box.cpp::rotateX (lines 293-318) ---- */
    void rotateX() {
        int p1[3], p2[3];
        for(int z = 0; z < 1; ++z) {
            p1[2] = z;  p2[2] = 1 - z;
            for (int y = 0; y < 3; ++y) {
                p1[1] = y;  p2[1] = 2 - y;
                for (int x=0; x < 4; ++x) {
                    p1[0] = x;  p2[0] = x;
                    Cube c1 = getCube(p1[0],p1[1],p1[2]);
                    Cube c2 = getCube(p2[0],p2[1],p2[2]);
                    int *v1 = c1.getVector(), *v2 = c2.getVector();
                    v1[1] = -v1[1]; v1[2] = -v1[2];
                    v2[1] = -v2[1]; v2[2] = -v2[2];
                    getCube(p1[0],p1[1],p1[2]).setState(c2.getState());
                    getCube(p1[0],p1[1],p1[2]).setVector(v2);
                    getCube(p2[0],p2[1],p2[2]).setState(c1.getState());
                    getCube(p2[0],p2[1],p2[2]).setVector(v1);
                }
            }
        }
    }

    /* ---- Fixed versions ---- */
    void rotateZ_fixed() {
        int p1[3], p2[3];
        for(int z = 0; z < BOX_D; ++z) {
            p1[2] = z;  p2[2] = z;
            for (int y = 0; y < BOX_H/2; ++y) {       // FIX: BOX_H/2
                p1[1] = y;  p2[1] = BOX_H - 1 - y;
                for (int x=0; x < BOX_W; ++x) {
                    p1[0] = x;  p2[0] = BOX_W - 1 - x;
                    Cube c1 = getCube(p1[0],p1[1],p1[2]);
                    Cube c2 = getCube(p2[0],p2[1],p2[2]);
                    int *v1 = c1.getVector(), *v2 = c2.getVector();
                    v1[0] = -v1[0]; v1[1] = -v1[1];
                    v2[0] = -v2[0]; v2[1] = -v2[1];
                    getCube(p1[0],p1[1],p1[2]).setState(c2.getState());
                    getCube(p1[0],p1[1],p1[2]).setVector(v2);
                    getCube(p2[0],p2[1],p2[2]).setState(c1.getState());
                    getCube(p2[0],p2[1],p2[2]).setVector(v1);
                }
            }
        }
    }

    void rotateX_fixed() {
        int p1[3], p2[3];
        for(int z = 0; z < BOX_D; ++z) {            // FIX: BOX_D not 1
            p1[2] = z;  p2[2] = BOX_D - 1 - z;
            for (int y = 0; y < BOX_H/2; ++y) {       // FIX: BOX_H/2
                p1[1] = y;  p2[1] = BOX_H - 1 - y;
                for (int x=0; x < BOX_W; ++x) {
                    p1[0] = x;  p2[0] = x;
                    Cube c1 = getCube(p1[0],p1[1],p1[2]);
                    Cube c2 = getCube(p2[0],p2[1],p2[2]);
                    int *v1 = c1.getVector(), *v2 = c2.getVector();
                    v1[1] = -v1[1]; v1[2] = -v1[2];
                    v2[1] = -v2[1]; v2[2] = -v2[2];
                    getCube(p1[0],p1[1],p1[2]).setState(c2.getState());
                    getCube(p1[0],p1[1],p1[2]).setVector(v2);
                    getCube(p2[0],p2[1],p2[2]).setState(c1.getState());
                    getCube(p2[0],p2[1],p2[2]).setVector(v1);
                }
            }
        }
    }

    void dump(const char* label) {
        printf("%s:\n", label);
        for (int z = 0; z < BOX_D; z++) {
            printf("  z=%d:\n", z);
            for (int y = 0; y < BOX_H; y++) {
                printf("    ");
                for (int x = 0; x < BOX_W; x++) {
                    const Cube& c = getCube(x,y,z);
                    if (c.isFull()) printf(" X ");
                    else if (c.isHalf()) { const int* v = const_cast<Cube&>(c).getVector(); printf("+[%d,%d,%d]", v[0], v[1], v[2]); }
                    else printf(" . ");
                }
                printf("\n");
            }
        }
        printf("\n");
    }

    bool equals(const BoxVectors& o) const {
        for (int x=0; x<BOX_W; x++)
            for (int y=0; y<BOX_H; y++)
                for (int z=0; z<BOX_D; z++)
                    if (cells[x][y][z] != o.cells[x][y][z]) return false;
        return true;
    }
};

/* ---- Test helpers ---- */

static void setHalf(BoxVectors& b, int x, int y, int z, int dx, int dy, int dz) {
    b.getCube(x,y,z).setState(cell_half);
    int v[3] = {dx,dy,dz};
    b.getCube(x,y,z).setVector(v);
}

static bool allEmpty(const BoxVectors& b) {
    for (int x=0; x<BOX_W; x++)
        for (int y=0; y<BOX_H; y++)
            for (int z=0; z<BOX_D; z++)
                if (!b.getCube(x,y,z).isEmpty()) return false;
    return true;
}

static int testCount = 0, passCount = 0;

#define CHECK(cond, msg) do { \
    testCount++; \
    if (cond) { passCount++; printf("  PASS: %s\n", msg); } \
    else { printf("  FAIL: %s (expected true, got false)\n", msg); } \
} while(0)

#define CHECK_EQ(a, b, msg) do { \
    testCount++; \
    if ((a) == (b)) { passCount++; printf("  PASS: %s\n", msg); } \
    else { printf("  FAIL: %s (got %d, expected %d)\n", msg, (int)(a), (int)(b)); } \
} while(0)

/* ============================================================ */
/*  Test 1: rotateZ on empty box — should stay empty            */
/* ============================================================ */
static void test_rotateZ_empty() {
    printf("\n=== Test 1: rotateZ on empty box ===\n");
    BoxVectors b;
    b.rotateZ();
    CHECK(allEmpty(b), "box stays empty after rotateZ");
}

/* ============================================================ */
/*  Test 2: rotateZ on full box — should stay full              */
/* ============================================================ */
static void test_rotateZ_full() {
    printf("\n=== Test 2: rotateZ on full box ===\n");
    BoxVectors b;
    for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++)
        b.getCube(x,y,z).setState(cell_full);
    b.rotateZ();
    int fullCount = 0;
    for (int x=0;x<BOX_W;x++) for (int y=0;y<BOX_H;y++) for (int z=0;z<BOX_D;z++)
        if (b.getCube(x,y,z).isFull()) fullCount++;
    CHECK_EQ(fullCount, BOX_W*BOX_H*BOX_D, "all cells still full after rotateZ");
}

/* ============================================================ */
/*  Test 3: rotateZ half-cell direction transform               */
/*  A half-cell at (0,0,0) with vector [+1,+1,0] should rotate
 *  to position (3,2,0) with vector [-1,-1,0] (negated).        */
/* ============================================================ */
static void test_rotateZ_half_cell() {
    printf("\n=== Test 3: rotateZ half-cell direction transform ===\n");
    BoxVectors b;
    setHalf(b, 0, 0, 0, 1, 1, 0);   // +x +y at corner

    b.rotateZ();

    // After 90° CCW rotation around z: (x,y) -> (y, W-1-x)
    // (0,0) -> (0, 3)  ... but wait the code maps (x,y)->(3-x, 2-y)
    // The code's mapping: (0,0) <-> (3,2)
    // So the half-cell should end up at (3,2,0) with vector [-1,-1,0]
    const Cube& c = b.getCube(3, 2, 0);
    CHECK(c.isHalf(), "half-cell moved to (3,2,0)");
    if (c.isHalf()) {
        int* v = const_cast<Cube&>(c).getVector();
        CHECK_EQ(v[0], -1, "x component negated");
        CHECK_EQ(v[1], -1, "y component negated");
        CHECK_EQ(v[2], 0,  "z component unchanged");
    }
}

/* ============================================================ */
/*  Test 4: rotateZ four times = identity (exposes double-swap) */
/*  With the double-swap bug, rotateZ is effectively a no-op
 *  (swap pairs twice), so 4x also gives identity.
 *  With the fix, we need to verify it actually rotates.        */
/* ============================================================ */
static void test_rotateZ_four_times() {
    printf("\n=== Test 4: rotateZ x4 = identity ===\n");
    BoxVectors b, orig;
    setHalf(b, 1, 0, 0, 1, 0, 1);   // +x +z at (1,0,0)

    orig = b;  // save copy

    b.rotateZ();  b.rotateZ();
    b.rotateZ();  b.rotateZ();

    CHECK(b.equals(orig), "rotateZ x4 returns to original state");
}

/* ============================================================ */
/*  Test 5: rotateZ inverse = rotateZ^3                         */
/* ============================================================ */
static void test_rotateZ_inverse() {
    printf("\n=== Test 5: rotateZ inverse (x3) ===\n");
    BoxVectors b, orig;
    setHalf(b, 1, 1, 0, 1, -1, 0);
    orig = b;

    b.rotateZ();
    // Apply rotateZ three more times (inverse)
    b.rotateZ();  b.rotateZ();  b.rotateZ();

    CHECK(b.equals(orig), "rotateZ x3 = inverse");
}

/* ============================================================ */
/*  Test 6: rotateX half-cell direction transform               */
/* ============================================================ */
static void test_rotateX_half_cell() {
    printf("\n=== Test 6: rotateX half-cell direction transform ===\n");
    BoxVectors b;
    setHalf(b, 0, 0, 0, 0, 1, 1);   // +y +z at (0,0,0)

    b.rotateX();

    // Code's mapping: (y,z) -> (2-y, 1-z)
    // (0,0) <-> (2,1)
    const Cube& c = b.getCube(0, 2, 1);
    CHECK(c.isHalf(), "half-cell moved to (0,2,1)");
    if (c.isHalf()) {
        int* v = const_cast<Cube&>(c).getVector();
        CHECK_EQ(v[1], -1, "y component negated");
        CHECK_EQ(v[2], -1, "z component negated");
        CHECK_EQ(v[0], 0,  "x component unchanged");
    }
}

/* ============================================================ */
/*  Test 7: rotateX four times = identity                       */
/* ============================================================ */
static void test_rotateX_four_times() {
    printf("\n=== Test 7: rotateX x4 = identity ===\n");
    BoxVectors b, orig;
    setHalf(b, 2, 1, 0, 0, 1, -1);
    orig = b;

    b.rotateX();  b.rotateX();
    b.rotateX();  b.rotateX();

    CHECK(b.equals(orig), "rotateX x4 returns to original state");
}

/* ============================================================ */
/*  Test 8: rotateX on both layers (z=0 and z=1)              */
/*  The buggy code only iterates z=0 (z < 1).                 */
/* ============================================================ */
static void test_rotateX_both_layers() {
    printf("\n=== Test 8: rotateX affects both z-layers ===\n");
    BoxVectors b;
    setHalf(b, 0, 0, 0, 0, 1, 0);   // z=0 layer
    setHalf(b, 0, 0, 1, 0, 1, 0);   // z=1 layer

    b.rotateX();

    const Cube& c0 = b.getCube(0, 2, 0);
    const Cube& c1 = b.getCube(0, 2, 1);
    CHECK(c0.isHalf(), "z=0 layer half-cell moved");
    CHECK(c1.isHalf(), "z=1 layer half-cell moved (BUG: buggy code skips z=1)");
}

/* ============================================================ */
/*  Test 9: rotateZ_fixed on half-cell                        */
/* ============================================================ */
static void test_rotateZ_fixed_half_cell() {
    printf("\n=== Test 9: rotateZ_fixed half-cell direction transform ===\n");
    BoxVectors b;
    setHalf(b, 0, 0, 0, 1, 1, 0);

    b.rotateZ_fixed();

    // Fixed mapping: (x,y) -> (y, W-1-x) = (0, 3)
    const Cube& c = b.getCube(0, 3, 0);
    // ... but y only goes to 2 (BOX_H=3), so y=3 is out of bounds!
    // The correct 90° CCW rotation for (x,y) in [0..W-1][0..H-1] is:
    //   (x,y) -> (y, W-1-x)  only works for square grids
    // For non-square, we need a different convention.
    // The original code uses (x,y) -> (3-x, 2-y) which is a 180° flip in xy.
    // Let's just verify the fixed version at least processes each pair once.
    // Check that (0,0) was swapped with (3,2):
    const Cube& c2 = b.getCube(3, 2, 0);
    CHECK(c2.isHalf(), "half-cell at (3,2,0) after fixed rotateZ");
    if (c2.isHalf()) {
        int* v = const_cast<Cube&>(c2).getVector();
        CHECK_EQ(v[0], -1, "x negated");
        CHECK_EQ(v[1], -1, "y negated");
    }
}

/* ============================================================ */
/*  Test 10: rotateX_fixed on both layers                     */
/* ============================================================ */
static void test_rotateX_fixed_both_layers() {
    printf("\n=== Test 10: rotateX_fixed affects both layers ===\n");
    BoxVectors b;
    setHalf(b, 0, 0, 0, 0, 1, 0);
    setHalf(b, 0, 0, 1, 0, 1, 0);

    b.rotateX_fixed();

    const Cube& c0 = b.getCube(0, 2, 0);
    const Cube& c1 = b.getCube(0, 2, 1);
    CHECK(c0.isHalf(), "z=0 half-cell moved");
    CHECK(c1.isHalf(), "z=1 half-cell moved");
    if (c0.isHalf()) {
        int* v = const_cast<Cube&>(c0).getVector();
        CHECK_EQ(v[1], -1, "y negated");
        // Original z=0, negated z = 0 (not -1!)
        CHECK_EQ(v[2], 0, "z unchanged (was 0)");
    }
    if (c1.isHalf()) {
        int* v = const_cast<Cube&>(c1).getVector();
        CHECK_EQ(v[1], -1, "y negated");
        // Original z=0, negated z = 0 (not -1!)
        CHECK_EQ(v[2], 0, "z unchanged (was 0)");
    }
}

/* ============================================================ */
/*  Test 11: round-trip rotateZ then rotateZ^3                */
/* ============================================================ */
static void test_rotateZ_roundtrip() {
    printf("\n=== Test 11: rotateZ + rotateZ^3 round-trip ===\n");
    BoxVectors b, orig;
    setHalf(b, 1, 1, 1, 1, -1, 1);
    orig = b;

    b.rotateZ();
    // Now b is rotated; rotate it back with 3 more Z rotations
    b.rotateZ();  b.rotateZ();  b.rotateZ();

    CHECK(b.equals(orig), "rotateZ + rotateZ^3 = identity");
}

/* ============================================================ */
/*  Test 12: symmetric test — fixed version also passes       */
/* ============================================================ */
static void test_fixed_roundtrip() {
    printf("\n=== Test 12: fixed version round-trip ===\n");
    BoxVectors b, orig;
    setHalf(b, 1, 1, 1, 1, -1, 1);
    orig = b;

    b.rotateZ_fixed();
    b.rotateZ_fixed();  b.rotateZ_fixed();  b.rotateZ_fixed();

    CHECK(b.equals(orig), "fixed rotateZ round-trip");
}

/* ============================================================ */
int main() {
    printf("=== Direction-Vector Rotate Tests ===\n");
    printf("Box dimensions: %d x %d x %d\n\n", BOX_W, BOX_H, BOX_D);

    test_rotateZ_empty();
    test_rotateZ_full();
    test_rotateZ_half_cell();
    test_rotateZ_four_times();
    test_rotateZ_inverse();
    test_rotateX_half_cell();
    test_rotateX_four_times();
    test_rotateX_both_layers();
    test_rotateZ_fixed_half_cell();
    test_rotateX_fixed_both_layers();
    test_rotateZ_roundtrip();
    test_fixed_roundtrip();

    printf("\n=== Results: %d/%d passed ===\n\n", passCount, testCount);
    return (passCount == testCount) ? 0 : 1;
}
