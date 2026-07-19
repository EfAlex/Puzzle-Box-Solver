# Optimization Plan: Rotate + Allocator Bottlenecks

## Goal

Eliminate the top 2 CPU bottlenecks identified by perf:
1. `figure::rotate()` — 23.1% CPU (heap allocations per rotation)
2. `addFigure`/`delFigure` cube methods — 9.3% CPU (ublas indirection on every placement)

---

## Current Bottleneck Data (perf, 5s delay + 1s record @ 10kHz)

| Rank | Function | % CPU | Root Cause |
|------|----------|-------|------------|
| 1 | `std::allocator<int>` | **36.8%** | Heap allocs from rotate() + solver state |
| 2 | `workerIterate` | **30.1%** | Solver core recursion loop |
| 3 | `figure::rotate` | **23.1%** | Allocates ublas vectors per rotation |
| 4 | `cube::isHalf` | 3.3% | Called millions of times |
| 5 | `cube::setVector` | 2.8% | ublas vector allocation |
| 6 | `cube::isFull` | 1.8% | |
| 7 | `cube::getVector` | 1.2% | ublas vector allocation |

Hot path: `workerIterate → addFigure → rotate → std::allocator`

---

## Figure Data Layout (current)

```
figure
  ├─ cubes: boost::ptr_vector<figure_cube>   (heap pointers!)
  │     └─ figure_cube
  │           ├─ cube c      (cell_t storage + int v[3] — v[] is stack)
  │           └─ int pos[3]
  └─ direction[2][3]: ublas::vector<int>     (heap!)

cube internals:
  ├─ cell_t storage
  └─ int v[3]          ← actual direction data lives here

cube::getVector() → returns int* to a **heap-allocated ublas vector copy** of v[3]
cube::setVector() → copies int* into a **heap-allocated ublas vector**
```

**Key insight:** `cube::v[3]` is already a stack array. `getVector()`/`setVector()` allocate a ublas vector copy each call. This is the root cause of the allocator bottleneck in `rotate()`.

---

## Option A: Fix `cube::getVector()` — Quick Win (Low Effort)

### Idea
`cube` already has `int v[3]` internally. Just return a direct pointer to it instead of allocating a ublas vector.

### Changes

**cube.hpp:**
```cpp
-   int *getVector();
-   bool setVector(const int *);
+   const int* getVector() const { return v; }
+   void setVector(const int (&vec)[3]) { v[0]=vec[0]; v[1]=vec[1]; v[2]=vec[2]; }
```

**figure.cpp rotate() — before:**
```cpp
void figure::rotate(const Mat3 &m) {
    BOOST_FOREACH(figure_cube & c, cubes) {
        std::array<int, 3> p = {{ i.pos[0], i.pos[1], i.pos[2] }};
        p = mat3_mul_vec(m, p);
        i.pos[0] = p[0]; i.pos[1] = p[1]; i.pos[2] = p[2];
        int *orig = i.c.getVector();                    // ← ublas alloc!
        std::array<int, 3> v = {{ orig[0], orig[1], orig[2] }};
        std::array<int, 3> res = mat3_mul_vec(m, v);
        int r[3]; r[0] = res[0]; r[1] = res[1]; r[2] = res[2];
        i.c.setVector(r);                              // ← ublas alloc!
    }
    // + 2 direction vectors with same ublas pattern
}
```

**figure.cpp rotate() — after:**
```cpp
void figure::rotate(const Mat3 &m) {
    BOOST_FOREACH(figure_cube & c, cubes) {
        std::array<int, 3> p = {{ c.pos[0], c.pos[1], c.pos[2] }};
        p = mat3_mul_vec(m, p);
        c.pos[0] = p[0]; c.pos[1] = p[1]; c.pos[2] = p[2];
        if (c.c.isHalf()) {
            auto res = mat3_mul_vec(m, c.c.getVector()); // ← direct int*
            c.c.setVector(res);                           // ← direct array ref
        }
    }
    // direction vectors — same pattern, no ublas allocs
}
```

**box.cpp addFigure/delFigure — before:**
```cpp
int *v1 = c.c.getVector();
const int *v2 = dirVec[code];
if (v1[0] != -v2[0] || v1[1] != -v2[1] || v1[2] != -v2[2]) return false;
```

**box.cpp — after:** same logic, but `getVector()` returns `v[3]` directly (no alloc).

### Expected Impact
- Eliminates ~30% of `std::allocator` overhead
- `rotate()` drops from 23% → ~16%
- Total speedup: **~15–20%**

### Risk
- **Low** — pure refactor, no logic change

### Effort
- **~1 hour**

---

## Option B: Precompute All 64 Rotations — Big Win (Medium Effort)

### Idea
Each figure has 4 cubes × 3 coords = 12 values. 4×4×4 = **64 rotation combinations per figure**. Precompute all 64 variants at init time. During solve, just index into the table — **zero mutation, zero rotation, zero allocation**.

### Data Structure
```cpp
struct FigureRotation {
    int pos[4][3];     // cube positions (max 4 cubes)
    int dir[4][3];     // direction vectors (0,0,0 for full cubes)
    uint8_t state[4];  // cell_t values
    uint8_t n_cubes;   // actual cube count
};
```

### Changes

**box_solver.hpp — add:**
```cpp
struct FigureRotation {
    int pos[4][3];
    int dir[4][3];
    uint8_t state[4];
    uint8_t n_cubes;
};

// In box_solver class:
FigureRotation precomputed_rotations[8][64];
uint8_t valid_rotation_count[8];  // how many unique orientations per figure
void precomputeRotations();
```

**box_solver.cpp — precomputeRotations():**
```cpp
void box_solver::precomputeRotations() {
    Mat3 rots[4] = { identity, rotZ90, rotY90, rotZm90 };
    for (int fig = 0; fig < 8; ++fig) {
        for (int rx = 0; rx < 4; ++rx)
        for (int ry = 0; ry < 4; ++ry)
        for (int rz = 0; rz < 4; ++rz) {
            Mat3 m = mat3_mul_vec(rots[rx], mat3_mul_vec(rots[ry], rots[rz]));
            auto &out = precomputed_rotations[fig][rx*16 + ry*4 + rz];
            out.n_cubes = figures[fig].cubes.size();
            for (int c = 0; c < out.n_cubes; ++c) {
                out.state[c] = figures[fig].cubes[c].c.getState();
                auto p = mat3_mul_vec(m, figures[fig].cubes[c].pos);
                out.pos[c][0] = p[0]; out.pos[c][1] = p[1]; out.pos[c][2] = p[2];
                if (figures[fig].cubes[c].c.isHalf()) {
                    auto d = mat3_mul_vec(m, figures[fig].cubes[c].c.getVector());
                    out.dir[c][0] = d[0]; out.dir[c][1] = d[1]; out.dir[c][2] = d[2];
                }
            }
        }
        // Deduplicate identical orientations
        valid_rotation_count[fig] = 0;
        for (int r = 0; r < 64; ++r) {
            bool unique = true;
            for (int r2 = 0; r2 < valid_rotation_count[fig]; ++r2) {
                if (memcmp(&precomputed_rotations[fig][r], &precomputed_rotations[fig][r2], sizeof(FigureRotation)) == 0) {
                    unique = false; break;
                }
            }
            if (unique) valid_rotation_count[fig]++;
        }
    }
}
```

**iterateFigure() / workerIterate() — before:**
```cpp
for (int rx = 0; rx < 4; ++rx)
for (int ry = 0; ry < 4; ++ry)
for (int rz = 0; rz < 4; ++rz) {
    // scan cubes to find boundaries (heap access)
    BOOST_FOREACH(figure_cube & c, f->cubes) { ... }
    // place figure (calls addFigure → getVector → ublas alloc)
    if (b.addFigure(*f, v)) { ... }
    // undo rotations at end of loops
    f->rotate(rotate_matrix[2]);
    f->rotate(rotate_matrix[1]);
    f->rotate(rotate_matrix[0]);
}
```

**iterateFigure() / workerIterate() — after:**
```cpp
for (int r = 0; r < valid_rotation_count[fig]; ++r) {
    const auto &rot = precomputed_rotations[fig][r];
    // find boundaries from precomputed pos[] (no heap access)
    for (int c = 0; c < rot.n_cubes; ++c) {
        min_v[0] = min(min_v[0], rot.pos[c][0]); ...
    }
    // place figure — new overload takes raw data
    if (b.addFigureFromRotation(rot, v)) { ... }
}
// NO rotation undo needed — precomputed data is immutable
```

**box.hpp/cpp — add `addFigureFromRotation()`:**
```cpp
// In box class:
bool addFigureFromRotation(const FigureRotation &rot, const vector_int &v);

// In box.cpp:
bool box::addFigureFromRotation(const FigureRotation &rot, const vector_int &v) {
    // Phase 1: validate (no allocations)
    for (int c = 0; c < rot.n_cubes; ++c) {
        int px = rot.pos[c][0] + v[0], py = rot.pos[c][1] + v[1], pz = rot.pos[c][2] + v[2];
        if (px < 0 || px >= BOX_W || py < 0 || py >= BOX_H || pz < 0 || pz >= BOX_D) return false;
        uint8_t code = cellState(px, py, pz);
        if (rot.state[c] == cell_full && code != 0) return false;
        if (rot.state[c] == cell_half && code == 1) return false;
        if (rot.state[c] == cell_half && code >= 2 && code <= 13) {
            if (rot.dir[c][0] != -dirVec[code][0] || ...) return false;
        }
    }
    // Phase 2: place (no allocations)
    for (int c = 0; c < rot.n_cubes; ++c) {
        int px = rot.pos[c][0] + v[0], ...;
        if (rot.state[c] == cell_full) setCellState(px,py,pz, 1);
        else if (rot.state[c] == cell_half) setCellState(px,py,pz, encodeDir(rot.dir[c][0], rot.dir[c][1], rot.dir[c][2]));
    }
    // Phase 3: check half-cube adjacency (no allocations)
    ...
    return true;
}
```

### Expected Impact
- Eliminates `figure::rotate()` entirely (23.1% → 0%)
- `addFigure` drops significantly (no `getVector`/`setVector` calls)
- Total speedup: **~30–50%**

### Risk
- **Medium** — changes solver loop structure, needs careful boundary computation from precomputed data
- Must verify deduplication doesn't miss any valid orientations

### Effort
- **~3 hours**

---

## Option C: Combined — A + B (Recommended)

Apply both optimizations. Option A is a prerequisite for clean Option B anyway (fix `getVector()` first, then switch to precomputed table).

| | Option A only | Option B only | **A + B (C)** |
|---|---|---|---|
| Effort | ~1 hr | ~3 hrs | **~3 hrs** |
| Speedup | ~15–20% | ~30–50% | **~40–60%** |
| Risk | Low | Medium | **Medium** |
| Files touched | cube.hpp/cpp, figure.cpp, box.cpp | box_solver.hpp/cpp, box.hpp/cpp, figure.cpp | **All above** |

---

## Implementation Order (Option C)

1. **cube.hpp/cpp** — fix `getVector()` / `setVector()` (Option A)
2. **figure.cpp** — simplify `rotate()` to use direct `int*` (Option A)
3. **box_solver.hpp** — add `FigureRotation` struct + precomputed table
4. **box_solver.cpp** — add `precomputeRotations()`, call after `initFigures()`
5. **box_solver.cpp** — rewrite `iterateFigure()` + `workerIterate()` to use table
6. **box.hpp/cpp** — add `addFigureFromRotation()` + `delFigureFromRotation()`
7. **Test** — verify solver finds same solutions
8. **Profile** — measure speedup with `./run-perf.sh 30`

---

## Files to Touch

| File | Changes |
|------|---------|
| `cube.hpp` | `getVector()` → `const int*`, `setVector()` → `const int (&)[3]` |
| `cube.cpp` | Implement new signatures |
| `figure.cpp` | Simplify `rotate()` — remove ublas indirection |
| `box_solver.hpp` | Add `FigureRotation` struct, `precomputed_rotations[8][64]`, `valid_rotation_count[8]` |
| `box_solver.cpp` | Add `precomputeRotations()`, rewrite `iterateFigure()` + `workerIterate()`, add `addFigureFromRotation()` helpers |
| `box.hpp` | Add `addFigureFromRotation()` / `delFigureFromRotation()` declarations |
| `box.cpp` | Implement `addFigureFromRotation()` / `delFigureFromRotation()` |

---

## Validation

- Solver must find **same solutions** as before
- Run `./run-perf.sh 30` before/after to measure speedup
- Expected: `std::allocator` drops from 36.8% → <10%, `figure::rotate` drops from 23.1% → 0%
