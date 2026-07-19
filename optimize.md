# Box Solver — Speed Optimization Proposals

## 1. Bitmask Occupancy Map (HIGH IMPACT)

### Problem
`box::content` is `ublas::vector<ublas::matrix<cube>>` — double heap indirection per cell. `addFigure`/`delFigure` iterate over figure cubes, constructing ublas indices and doing bounds checks each time.

### Proposal
Replace with a flat `uint64_t` bitmask (24 cells fit in one 64-bit word):

```cpp
// Before: ublas::vector<ublas::matrix<cube>> content;
uint64_t occupancy;  // 1 word for 4×3×2 = 24 cells

static constexpr int CELL_BITS = 24;  // N*N*M
static constexpr int WORDS = 1;

bool isOccupied(int x, int y, int z) const {
    return (occupancy >> ((x + y*4 + z*4*3))) & 1ULL;
}

void setOccupied(int x, int y, int z) {
    occupancy |= 1ULL << (x + y*4 + z*4*3);
}

void clearOccupied(int x, int y, int z) {
    occupancy &= ~(1ULL << (x + y*4 + z*4*3));
}
```

For `addFigure`: precompute a `uint64_t mask` per figure per orientation, then:
```cpp
bool canPlace(const uint64_t& figureMask, int offset) {
    return ((occupancy ^ (figureMask << offset)) & figureMask) == 0;
    // i.e., no overlap between figure cells and occupied cells
}
```

**Expected speedup: 10–50×** for occupancy checks.

---

## 2. Precomputed Figure Masks per Orientation (HIGH IMPACT)

### Problem
`addFigure` computes world-space positions from local figure cells on every call. 64 rotations × 24 positions × 8 figures = thousands of redundant computations.

### Proposal
**Precompute** at startup:
- For each figure, for each valid rotation, the world-space bitmask for every valid anchor position.
- Bounding box (min/max x, y, z) to skip invalid anchors.

```cpp
struct PrecomputedOrientation {
    uint64_t masks[24];  // mask for each anchor position (x,y,z)
    int validAnchors;    // how many of the 24 are valid
    int minX, maxX, minY, maxY, minZ, maxZ;
};

// One-time setup:
for (auto* fig : figures) {
    for (int rx = 0; rx < 4; rx++) {
        for (int ry = 0; ry < 4; ry++) {
            for (int rz = 0; rz < 4; rz++) {
                auto cells = rotate(fig->cubes, rx, ry, rz);
                auto bbox = computeBBox(cells);
                if (fitsInBox(bbox)) {
                    for each valid anchor (ax, ay, az):
                        precomputed.orientations.push_back({
                            .masks = computeMasks(cells, ax, ay, az),
                            .bbox = bbox,
                        });
                }
            }
        }
    }
}
```

Then `iterateFigure` inner loop becomes:
```cpp
for (auto& po : fig->precomputed) {
    for (int a = 0; a < po.validAnchors; a++) {
        if (po.masks[a] & occupancy) continue;  // collision
        // place: occupancy |= po.masks[a];
        // recurse...
        // undo: occupancy ^= po.masks[a];  // XOR for idempotent undo
    }
}
```

**Expected speedup: 5–15×** for the inner loop.

---

## 3. Most-Constrained-First Figure Ordering (MEDIUM-HIGH IMPACT)

### Problem
Figures are processed in declaration order (0→7). Some figures have far fewer valid placements and should go first to prune the tree early.

### Proposal
Compute a **constrainedness score** for each figure (fewer valid positions = higher priority) and reorder before solving:

```cpp
struct FigureOrder {
    int originalIndex;
    int validPlacementCount;  // precomputed estimate
};

// Score by: cube_count (ascending) * orientation_diversity
for (int i = 0; i < figures.size(); i++) {
    order[i] = {i, estimateValidPlacements(figures[i])};
}
std::sort(order.begin(), order.end(), [](auto& a, auto& b) {
    return a.validPlacementCount < b.validPlacementCount;
});

// Reorder figures and rotation matrices by sorted order
```

Heuristic: sort by **ascending number of cubes** (smallest figures are most constrained in a fixed box), breaking ties by **orientation diversity** (fewer unique orientations first).

**Expected speedup: 2–5×** for heterogeneous figure sets.

---

## 4. Increase spawnDepth / Top-Level Parallelism (MEDIUM IMPACT)

### Problem
`spawnDepth = 1` means the master handles only figure 0, then spawns workers for figures 1–7. This creates unbalanced work: figure 0's branching factor determines thread work distribution, and workers don't spawn further.

### Proposal
**Raise `spawnDepth` to 2 or 3:**
```cpp
int spawnDepth = 2;  // master handles figures 0, 1; workers start at figure 2
```

This gives workers larger independent subproblems and reduces thread spawn overhead. Combined with top-level parallelism:

```cpp
// Split figure 0's positions across threads
int nThreads = std::thread::hardware_concurrency();
int chunk = (firstFig->orientations.size() + nThreads - 1) / nThreads;
for (int t = 0; t < nThreads; t++) {
    threads.emplace_back([t, chunk]() {
        for (int i = t*chunk; i < min((t+1)*chunk, total); i++) {
            applyOrientation(i);
            for (auto& pos : validPositions) {
                if (canPlace(pos)) {
                    place(pos);
                    workerIterate(1, snapshot);  // recurse on worker
                    unplace(pos);
                }
            }
        }
    });
}
```

**Expected speedup: ~1.5–2×** per doubling of spawnDepth, diminishing after spawnDepth=3.

---

## 5. Eliminate Atomic Counters / False Sharing (MEDIUM IMPACT)

### Problem
`shared.count.fetch_add(1, relaxed)` fires on every placement across all threads → cache line bouncing. `shared.depth[8]` has 8 atomics on the same cache line → false sharing.

### Proposal
**Per-thread counters:**
```cpp
// Thread-local
thread_local long threadCount = 0;

// Inner loop:
threadCount++;
if (threadCount >= check_count) {
    shared.count.fetch_add(threadCount);
    threadCount = 0;
    emitStatus();
}
```

**Pad depth array:**
```cpp
struct PaddedAtomic {
    std::atomic<long> val;
    char pad[64 - sizeof(std::atomic<long>)];
};
PaddedAtomic depth[8];
```

**Expected speedup: 1.5–3×** for multi-threaded runs (reduces contention).

---

## 6. Forward Checking (MEDIUM IMPACT)

### Problem
`isSolvable` only checks volume upper bounds. It doesn't detect when a remaining figure has **no valid position** at all.

### Proposal
After each placement, verify every remaining figure has at least one valid position:

```cpp
bool forwardCheck(int level, uint64_t occupancy) {
    for (int i = level; i < figures.size(); i++) {
        bool hasAny = false;
        for (auto& po : figures[i]->precomputed) {
            for (int a = 0; a < po.validAnchors; a++) {
                if (!(po.masks[a] & occupancy)) {
                    hasAny = true; break;
                }
            }
            if (hasAny) break;
        }
        if (!hasAny) return false;  // dead end
    }
    return true;
}
```

**Expected speedup: 1.5–3×** for hard instances with many dead ends.

---

## 7. Replace ublas with Fixed-Size Arrays (MEDIUM IMPACT)

### Problem
`rotate_matrix` is `vector<ublas::matrix<int>>` — dynamic heap allocation per matrix. Rotation matrices are always 3×3 and used in the innermost loop.

### Proposal
```cpp
// Before:
using rotate_matrix = vector<ublas::matrix<int>>;

// After:
using mat3 = std::array<std::array<int, 4>, 4>;  // 4×4 for homogeneous coords
static constexpr mat3 RX[4], RY[4], RZ[4];  // compile-time constants
```

This eliminates heap allocations in the hot path and improves compiler auto-vectorization.

**Expected speedup: 1.2–2×** for rotation-heavy workloads.

---

## 8. SIMD Vectorization (MEDIUM IMPACT)

### Problem
Inner loops check figure cells sequentially. For 4-cube figures, this is tight but not auto-vectorizable due to ublas overhead.

### Proposal
With bitmask occupancy (optimization #1), natural SIMD emerges:

```cpp
#include <immintrin.h>

// Check 4 anchor positions simultaneously
__m128i occ = _mm_load_si128((__m128i*)&occupancy);
__m128i results = _mm_setzero_si128();
for (int a = 0; a < validAnchors; a += 4) {
    __m128i mask = loadMasks[a];
    __m128i shifted = _mm_slli_epi64(mask, anchorOffset[a]);
    __m128i collision = _mm_and_si128(occ, shifted);
    results = _mm_or_si128(results, _mm_cmpeq_epi32(collision, _mm_setzero_si128()));
}
// results now has 4 valid/invalid flags
```

**Expected speedup: 2–4×** for large figures on wide cubes.

---

## 9. Symmetry Reduction (MEDIUM-HIGH IMPACT — RESEARCH REQUIRED)

### Problem
The 4×3×2 box has symmetries (rotations/reflections). The solver may find symmetrically equivalent solutions that could be pruned.

### Proposal
Precompute the **symmetry group** of the box (size ≤ 8 for a rectangular prism). For each solution found, generate all symmetric variants and reject if any already exists. Alternatively, **constrain the first figure's placements** to one representative per symmetry class.

**Expected speedup: 2–8×** depending on box symmetry.

---

## 10. Early Exit After First Solution (TRIVIAL)

If you only need one solution:

```cpp
std::atomic<bool> found{false};

// In leaf success path:
if (found.exchange(true)) return;
addSolution();

// In inner loops:
if (found.load()) break;
```

**Expected speedup: infinite** (stops immediately vs exploring entire tree).

---

## 11. Compile Optimizations (LOW EFFORT, GUARANTEED GAIN)

```bash
# CMakeLists.txt or qmake:
QMAKE_CXXFLAGS += -O3 -march=native -funroll-loops -ffast-math
QMAKE_CXXFLAGS -= -g  # release build
```

- `-O3`: aggressive inlining, vectorization
- `-march=native`: use AVX2/AVX-512 instructions on your CPU
- `-funroll-loops`: the fixed 64-iteration rotation loop unrolls well

---

## 12. Work Queue / Task Stealing (MEDIUM EFFORT)

### Problem
Current threading is a one-time fork: master handles figures 0..spawnDepth-1, workers handle the rest. Workers may finish early while others are overloaded.

### Proposal
Replace `feedThread()` with a **shared work queue**:

```cpp
class WorkQueue {
    std::queue<SubTask> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};

    void push(SubTask t) { lock_guard, push, notify_one; }
    bool pop(SubTask& out) { lock_guard, if empty wait, else pop; }
};
```

Workers pull remaining subtasks from the queue. When a worker finishes its figure level, it pushes remaining subtasks back.

**Expected speedup: 1.2–1.5×** over current approach (better load balancing).

---

## Priority Roadmap

| Phase | Optimization | Effort | Gain |
|-------|-------------|--------|------|
| **1** | #10 Early exit (if applicable) | Trivial | ∞ |
| **1** | #11 Compile opts (-O3 -march=native) | Trivial | 1.2–1.5× |
| **1** | #3 MRV figure ordering | Low | 2–5× |
| **2** | #1 Bitmask occupancy | Medium | 10–50× |
| **2** | #2 Precomputed figure masks | Medium | 5–15× |
| **3** | #5 Per-thread counters | Low | 1.5–3× |
| **3** | #6 Forward checking | Low | 1.5–3× |
| **3** | #7 Replace ublas mat3 | Low | 1.2–2× |
| **4** | #4 Increase spawnDepth | Low | 1.5–2× |
| **4** | #8 SIMD | Medium | 2–4× |
| **5** | #9 Symmetry reduction | Medium | 2–8× |
| **5** | #12 Work queue | Medium | 1.2–1.5× |

**Start with Phase 1** (trivial gains), then **Phase 2** (bitmask + precomputed masks — the biggest wins). These two together should give **50–100×** speedup for the occupancy check path.
