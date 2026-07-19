# Plan: Replace `ublas::matrix<int>` with fixed-size arrays

## Goal

Eliminate heap allocations and dynamic dispatch from `boost::numeric::ublas::matrix<int>` in the hot path (figure rotation). Replace with `std::array<std::array<int, 3>, 3>` (stack-allocated, cache-friendly, no allocations).

## Why it matters

- `ublas::matrix<int>` allocates on the heap on construction — every `matrix<int> r(3, 3)` in the innermost loop triggers an allocation.
- `figure::rotate(matrix<int> m)` takes the matrix **by value**, so it copies the heap buffer on every call.
- The rotation happens in the innermost loop: **64× per figure × 8 figures = 512+ times per search level**, across all threads.
- `prod(m, p)` does a matrix-vector multiply that ublas dispatches through virtual-like indirection.

## Scope: What changes, what stays

### In scope (replace `ublas::matrix<int>`)

| # | Candidate | Current type | Target type | Frequency |
|---|---|---|---|---|
| 1 | `rotate_matrix` member | `std::vector<ublas::matrix<int>>` | `std::array<Mat3, 3>` | 3 elements, static |
| 2 | `WorkerIterateState::rotateMatrix` | `const std::vector<ublas::matrix<int>>&` | `const std::array<Mat3, 3>&` | passed to workers |
| 3 | `setRotateVector()` param | `ublas::matrix<int>&` | `Mat3&` | called 8× in initSolution |
| 4 | `figure::rotate()` param | `ublas::matrix<int>` (by value) | `Mat3` (by value) | **innermost loop — every rotation** |
| 5 | `matrix<int> r(3,3)` locals in `init()` | ublas heap matrix | `Mat3` on stack | 1× at startup |
| 6 | `matrix<int> r(3,3)` locals in `initSolution()` | ublas heap matrix | `Mat3` on stack | 1× (debug/helper only) |

### Out of scope (leave as ublas)

| # | Candidate | Reason |
|---|---|---|
| 1 | `box::content` (`ublas::vector<ublas::matrix<cube>>`) | Stores `cube` objects (not ints), different optimization target (see `optimize.md` item #5) |
| 2 | `box_solution` uses | Not relevant to this change |

## Files to modify

| File | Changes |
|---|---|
| `common.hpp` | Add `using Mat3 = std::array<std::array<int, 3>, 3>;` and a `mat3_mul_vec()` helper |
| `figure.hpp` | Change `rotate(matrix<int>)` → `rotate(Mat3)` |
| `figure.cpp` | Replace `prod(m, p)` with `mat3_mul_vec(m, p)`; replace `BOOST_FOREACH` with range-for |
| `box_solver.hpp` | Change `rotate_matrix` type, `setRotateVector` signature, `WorkerIterateState::rotateMatrix` type |
| `box_solver.cpp` | Update `init()`, `setRotateVector()`, `initSolution()`, `iterateFigure()`, `workerIterate()` |

## Detailed changes

### 1. `common.hpp` — define `Mat3` and helpers

```cpp
#include <array>

using Mat3 = std::array<std::array<int, 3>, 3>;

// Matrix × vector multiply: result = m × v (column vector)
inline vector_int mat3_mul_vec(const Mat3& m, const vector_int& v) {
    vector_int r(3);
    r[0] = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2];
    r[1] = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2];
    r[2] = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2];
    return r;
}

// Zero-initialize a Mat3
inline void mat3_zero(Mat3& m) {
    for (auto& row : m) row.fill(0);
}

// Set identity
inline void mat3_identity(Mat3& m) {
    for (auto& row : m) row.fill(0);
    m[0][0] = m[1][1] = m[2][2] = 1;
}
```

### 2. `figure.hpp` — change rotate signature

```cpp
// Before:
void rotate(boost::numeric::ublas::matrix < int >);

// After:
void rotate(const Mat3& m);
```

### 3. `figure.cpp` — replace `figure::rotate()` implementation

```cpp
// Before (figure.cpp:88-114):
void figure::rotate(matrix < int >m)
{
    BOOST_FOREACH(figure_cube & i, cubes) {
        vector_int p (3);
        p[0] = i.pos[0]; p[1] = i.pos[1]; p[2] = i.pos[2];
        p = prod(m, p);
        i.pos[0] = p[0]; i.pos[1] = p[1]; i.pos[2] = p[2];
        int *orig = i.c.getVector();
        vector_int v (3);
        v[0] = orig[0]; v[1] = orig[1]; v[2] = orig[2];
        vector_int res (3);
        res = prod(m, v);
        int r[3];
        r[0] = res[0]; r[1] = res[1]; r[2] = res[2];
        i.c.setVector(r);
    }
    direction[0] = prod(m, direction[0]);
    direction[1] = prod(m, direction[1]);
}

// After:
void figure::rotate(const Mat3& m)
{
    for (auto& i : cubes) {
        // Rotate position
        int np[3];
        np[0] = m[0][0]*i.pos[0] + m[0][1]*i.pos[1] + m[0][2]*i.pos[2];
        np[1] = m[1][0]*i.pos[0] + m[1][1]*i.pos[1] + m[1][2]*i.pos[2];
        np[2] = m[2][0]*i.pos[0] + m[2][1]*i.pos[1] + m[2][2]*i.pos[2];
        i.pos[0] = np[0]; i.pos[1] = np[1]; i.pos[2] = np[2];

        // Rotate direction vector
        int* orig = i.c.getVector();
        int nv[3];
        nv[0] = m[0][0]*orig[0] + m[0][1]*orig[1] + m[0][2]*orig[2];
        nv[1] = m[1][0]*orig[0] + m[1][1]*orig[1] + m[1][2]*orig[2];
        nv[2] = m[2][0]*orig[0] + m[2][1]*orig[1] + m[2][2]*orig[2];
        i.c.setVector(nv);
    }
    // Rotate basis vectors
    int d0r[3], d1r[3];
    d0r[0] = m[0][0]*direction[0][0] + m[0][1]*direction[0][1] + m[0][2]*direction[0][2];
    d0r[1] = m[1][0]*direction[0][0] + m[1][1]*direction[0][1] + m[1][2]*direction[0][2];
    d0r[2] = m[2][0]*direction[0][0] + m[2][1]*direction[0][1] + m[2][2]*direction[0][2];
    direction[0][0] = d0r[0]; direction[0][1] = d0r[1]; direction[0][2] = d0r[2];

    d1r[0] = m[0][0]*direction[1][0] + m[0][1]*direction[1][1] + m[0][2]*direction[1][2];
    d1r[1] = m[1][0]*direction[1][0] + m[1][1]*direction[1][1] + m[1][2]*direction[1][2];
    d1r[2] = m[2][0]*direction[1][0] + m[2][1]*direction[1][1] + m[2][2]*direction[1][2];
    direction[1][0] = d1r[0]; direction[1][1] = d1r[1]; direction[1][2] = d1r[2];
}
```

### 4. `box_solver.hpp` — update types

```cpp
// Before:
std::vector < figure > figures, solution;
std::vector < vector_int >solution_pos;
std::vector < boost::numeric::ublas::matrix < int > >rotate_matrix;
...
void setRotateVector(boost::numeric::ublas::matrix < int > & , int, int, int, int, int, int);
...
struct WorkerIterateState {
    ...
    const std::vector<boost::numeric::ublas::matrix<int>> &rotateMatrix;
    ...
};

// After:
std::vector < figure > figures, solution;
std::vector < vector_int >solution_pos;
std::array<Mat3, 3> rotate_matrix;
...
void setRotateVector(Mat3&, int, int, int, int, int, int);
...
struct WorkerIterateState {
    ...
    const std::array<Mat3, 3>& rotateMatrix;
    ...
};
```

### 5. `box_solver.cpp` — update implementations

#### `init()` — replace ublas matrix construction with Mat3

```cpp
// Before (init lines 54-70):
rotate_matrix.resize(0);
matrix < int >r(3, 3);
r(0, 0) =  1; r(0, 1) =  0; r(0, 2) =  0;
r(1, 0) =  0; r(1, 1) =  0; r(1, 2) = -1;
r(2, 0) =  0; r(2, 1) =  1; r(2, 2) =  0;
rotate_matrix.push_back(r);
// ... 2 more matrices ...

// After:
rotate_matrix.fill({{}});  // zero-init all

// Matrix 0: Z-axis 90° rotation
{
    auto& m = rotate_matrix[0];
    m[0][0]= 1; m[0][1]= 0; m[0][2]= 0;
    m[1][0]= 0; m[1][1]= 0; m[1][2]=-1;
    m[2][0]= 0; m[2][1]= 1; m[2][2]= 0;
}
// Matrix 1: Y-axis 90° rotation
{
    auto& m = rotate_matrix[1];
    m[0][0]= 0; m[0][1]= 0; m[0][2]= 1;
    m[1][0]= 0; m[1][1]= 1; m[1][2]= 0;
    m[2][0]=-1; m[2][1]= 0; m[2][2]= 0;
}
// Matrix 2: X-axis 90° rotation
{
    auto& m = rotate_matrix[2];
    m[0][0]= 0; m[0][1]=-1; m[0][2]= 0;
    m[1][0]= 1; m[1][1]= 0; m[1][2]= 0;
    m[2][0]= 0; m[2][1]= 0; m[2][2]= 1;
}
```

#### `setRotateVector()` — replace ublas indexing with array indexing

```cpp
// Before:
void box_solver::setRotateVector(boost::numeric::ublas::matrix < int > & m, int x1, int y1, int z1, int x2, int y2 , int z2)
{
    m(0,0) = x1; m(1,0) = y1; m(2,0) = z1;
    m(0,1) = x2; m(1,1) = y2; m(2,1) = z2;
    m(0,2) = y1*z2 - z1*y2; m(1,2) = z1*x2 - x1*z2; m(2,2) = x1*y2 - y1*x2;
}

// After:
void box_solver::setRotateVector(Mat3& m, int x1, int y1, int z1, int x2, int y2, int z2)
{
    m[0][0] = x1; m[1][0] = y1; m[2][0] = z1;
    m[0][1] = x2; m[1][1] = y2; m[2][1] = z2;
    m[0][2] = y1*z2 - z1*y2; m[1][2] = z1*x2 - x1*z2; m[2][2] = x1*y2 - y1*x2;
}
```

#### `initSolution()` — replace ublas local with Mat3

```cpp
// Before:
matrix < int >r(3, 3);
setRotateVector(r, 0,1,0,-1,0,0);
solution[0] = figures[0]; solution[0].rotate(r);
// ... 7 more ...

// After:
Mat3 r;
mat3_zero(r);
setRotateVector(r, 0,1,0,-1,0,0);
solution[0] = figures[0]; solution[0].rotate(r);
// ... same pattern for remaining 7 ...
```

#### `iterateFigure()` — replace `rotate_matrix[N]` with `rotate_matrix[N]` (same name, different type)

No signature change needed — the loop `f->rotate(rotate_matrix[2])` works identically since `rotate()` now takes `const Mat3&`.

#### `workerIterate()` — same, no change needed at call sites

The `rotateMatrix[N]` indexing works the same for `std::array`.

#### Remove ublas include

Remove `#include <boost/numeric/ublas/matrix.hpp>` from `box_solver.hpp` and `figure.hpp` (if no longer needed).

## Expected performance impact

| Metric | Before | After |
|---|---|---|
| Heap allocs per rotation | 1 (construction) + 1 (copy) = 2 | 0 |
| `prod()` dispatch | ublas virtual-like indirection | inline arithmetic |
| Cache locality | heap pointer chase | L1 cache (36 bytes) |
| `setRotateVector` writes | heap store per call | stack store (eliminated by compiler) |

**Rough estimate**: The hot path (`figure::rotate` in innermost loop) goes from ~200ns (alloc + copy + ublas dispatch) to ~20ns (pure arithmetic). For 500M+ rotations across the search, this saves **hours** of wall time.

## Risk / verification

1. **`figure::rotate` semantics**: Must verify the matrix-vector multiply order matches ublas `prod(m, v)` (column-vector convention: `r[i] = sum_j m[i][j] * v[j]`). The inline version above uses this convention.
2. **`direction` rotation**: ublas `direction[0] = prod(m, direction[0])` returns a vector assigned to another ublas vector. The inline version writes to a temp array then assigns element-by-element. Verify this produces identical results.
3. **`setRotateVector` column-major**: ublas uses column-major `m(row, col)`. The Mat3 uses `m[row][col]` — same indexing. Verify the three precomputed matrices produce identical rotations.
4. **Build test**: `make clean && make` — verify no compilation errors.
5. **Functional test**: Run solver, verify solution output matches before/after.

## Implementation order

1. `common.hpp` — add `Mat3` type + helpers
2. `figure.hpp` — change `rotate()` signature
3. `figure.cpp` — rewrite `figure::rotate()` body
4. `box_solver.hpp` — update `rotate_matrix`, `setRotateVector`, `WorkerIterateState` types
5. `box_solver.cpp` — update `init()`, `setRotateVector()`, `initSolution()` bodies
6. Remove `#include <boost/numeric/ublas/matrix.hpp>` from `box_solver.hpp` and `figure.hpp` if safe
7. Build + test
