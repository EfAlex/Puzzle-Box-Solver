# Rework `isUnique` — Figure-Level Comparison with Symmetry Transforms

## Bug Fix (rotateZX corrected)

The original `rotateZX` entry was identical to `rotateZ` (both computed `(3-x, 2-y, z)`).
The correct `rotateZX = rotateX ∘ rotateZ` maps `(x,y,z) -> (3-x, y, 1-z)`.

This was causing the solver to test the same symmetry twice and skip `(3-x, y, 1-z)`,
which could lead to duplicate solutions being incorrectly accepted as unique.

## Goal

Remove all `Box` class operations (`addFigure`, `delFigure`, `box == box` mask comparison) from `isUnique()` in `box_solution.cpp`. Replace with figure-level comparison using precomputed symmetry transforms on both position vectors (`np`) and halfcube direction vectors.

## Current `isUnique` (box_solution.cpp:79-118)

```cpp
bool box_solution::isUnique(std::vector < figure > nf, const std::vector < vector_int >np)
{
    std::lock_guard<std::mutex> lock(solutionMutex);
    box b1;
    box_solution bs1(nf, np);
    for (unsigned int i = 0; i < nf.size(); ++i) {
        b1.addFigure(nf[i], np[i]);  // builds occupancy mask
    }
    BOOST_FOREACH(box_solution & bs2, solution_list) {
        box b2;
        std::vector < figure > s;
        std::vector < vector_int > sp;
        bs2.to_vector(s, sp);
        for (unsigned int i = 0; i < bs2.solution.size(); ++i) {
            b2.addFigure(s[i], sp[i]);  // builds occupancy mask
        }
        for (unsigned int i = 0; i < nf.size(); i++) {
            box cb1(b1);
            box cb2(b2);
            cb1.delFigure(nf[i], np[i]);  // remove figure i from mask
            cb2.delFigure(s[i], sp[i]);
            if (cb1 == cb2) return false;  // mask comparison (12 bytes)
            cb2.rotateZ();
            if (cb1 == cb2) return false;
            cb2.rotateX();
            if (cb1 == cb2) return false;
            cb2.rotateZ();
            if (cb1 == cb2) return false;
        }
    }
    return true;
}
```

**What it does**: Rebuilds `Box` occupancy masks for both solutions, then for each figure index i, removes figure i from both masks and checks if remaining masks are equal under 4 symmetries (identity, rotateZ, rotateX, rotateZX).

## Box Symmetry Group (D2)

Box dimensions: `BOX_W=4, BOX_H=3, BOX_D=2` (x, y, z)

Four symmetry operations mapping the box onto itself:

| Symmetry | Position transform: (x,y,z) -> | sign | offset | dsign (direction) |
|-----------|-------------------------------|------|--------|--------------------|
| identity | (x, y, z) | {1, 1, 1} | {0, 0, 0} | {1, 1, 1} |
| rotateZ | (3-x, 2-y, z) | {-1, -1, 1} | {3, 2, 0} | {-1, -1, 1} |
| rotateX | (x, 2-y, 1-z) | {1, -1, -1} | {0, 2, 1} | {1, -1, -1} |
| rotateZX | (3-x, y, 1-z) | {-1, 1, -1} | {3, 0, 1} | {-1, 1, -1} |

Derived from existing `box::rotateZ()` and `box::rotateX()` in `box.cpp`:
- `rotateZ`: 180° in xy plane: `(x,y) -> (BOX_W-1-x, BOX_H-1-y)` = `(3-x, 2-y)`
- `rotateX`: 180° in yz plane: `(y,z) -> (BOX_H-1-y, BOX_D-1-z)` = `(2-y, 1-z)`
- `rotateZX` = compose rotateZ then rotateX: `(3-x, 2-y, 1-z)`

## New Data Structure (`box_solver.hpp`)

```cpp
// Box symmetry — D2 group element
struct BoxSymmetry {
    int sign[3];       // position sign: symPos[i] = sign[i] * p[i] + offset[i]
    int offset[3];
    int dsign[3];      // direction sign: symDir[i] = dsign[i] * dir[i]
};

static const BoxSymmetry boxSymmetries[4] = {
    // identity: (x, y, z) -> (x, y, z)
    { {1, 1, 1}, {0, 0, 0}, {1, 1, 1} },

    // rotateZ: 180° in xy plane: (x,y,z) -> (3-x, 2-y, z)
    { {-1,-1, 1}, {3, 2, 0}, {-1,-1, 1} },

    // rotateX: 180° in yz plane: (x,y,z) -> (x, 2-y, 1-z)
    { { 1,-1,-1}, {0, 2, 1}, { 1,-1,-1} },

    // rotateZX: compose rotateZ + rotateX: (x,y,z) -> (3-x, y, 1-z)
    { {-1, 1,-1}, {3, 0, 1}, {-1, 1,-1} },
};

// Forward transform: symPos[i] = sign[i] * p[i] + offset[i]
// Inverse transform: invPos[i] = sign[i] * (p[i] - offset[i])  (signs are self-inverse)
```

## New `isUnique` Implementation (`box_solution.cpp`)

### Comparison Algorithm

For each existing solution, for each figure index i (removing corresponding figures):
1. Collect remaining positions: `remPos` = all positions except index i
2. Collect remaining halfcube directions: `remDirs` = first halfcube direction from each figure except i
3. For each of 4 symmetries:
   - Transform `remPos` using position symmetry
   - Transform `remDirs` using direction symmetry
   - Check if transformed `remPos` matches new `np` (remaining positions)
   - Check if transformed `remDirs` matches new halfcube directions
   - If all match under one symmetry → solutions equivalent → return `false`
4. If no symmetry matches for any i → return `true` (unique)

### Position Matching Logic

For each remaining position `sp[j]` (existing solution), transform and check against any `np[k]` (new solution):
```
for each symmetry:
    matched = 0
    for each remaining j != i:
        found = false
        for each k != i:
            if np[k] already matched to some j: continue
            symPos[d] = sign[d] * sp[j][d] + offset[d]
            if symPos == np[k]: found = true; mark np[k] as used; matched++
        if not found: break
    if matched == remaining_count: return false  // match found
return true
```

### Halfcube Direction Matching Logic

For each remaining figure j != i, find first halfcube in figure, get its direction vector:
```cpp
// Find first halfcube in figure j, get its direction
int halfcubeDir[3] = {0, 0, 0};
bool foundHalfcube = false;
for (int c = 0; c < s[j].n_cubes && !foundHalfcube; ++c) {
    if (s[j].cubes[c].c.isHalf()) {
        const int *dv = s[j].cubes[c].c.getVector();
        halfcubeDir[0] = dv[0];
        halfcubeDir[1] = dv[1];
        halfcubeDir[2] = dv[2];
        foundHalfcube = true;
    }
}

// Transform direction under symmetry
int symDir[3];
symDir[d] = dsign[d] * halfcubeDir[d];

// Compare with corresponding figure in new solution
// Find first halfcube in nf[k] and compare transformed direction
```

### Complete Pseudocode

```cpp
bool box_solution::isUnique(std::vector<figure> nf, const std::vector<vector_int> np)
{
    std::lock_guard<std::mutex> lock(solutionMutex);

    // Count remaining figures (excluding index i)
    int n_remaining = static_cast<int>(nf.size()) - 1;
    if (n_remaining <= 0) {
        // With 0 or 1 remaining figures, mask comparison is the only way
        // Fall back to empty-box comparison
        return true;  // single figure always unique (no symmetry to apply)
    }

    BOOST_FOREACH(const box_solution & bs2, solution_list) {
        std::vector<figure> s;
        std::vector<vector_int> sp;
        bs2.to_vector(s, sp);

        for (int i = 0; i < static_cast<int>(nf.size()); ++i) {
            // Collect remaining positions and directions (excluding index i)
            std::vector<int> remPosIdx;  // indices j != i
            for (int j = 0; j < static_cast<int>(nf.size()); ++j) {
                if (j != i) remPosIdx.push_back(j);
            }

            for (int sym = 0; sym < 4; ++sym) {
                const BoxSymmetry &bs = boxSymmetries[sym];

                // For each remaining position in existing solution,
                // transform and try to match to a remaining position in new solution
                int matched = 0;
                bool posMatch = true;
                std::vector<bool> npUsed(nf.size(), false);

                for (int ri = 0; ri < static_cast<int>(remPosIdx.size()); ++ri) {
                    int j = remPosIdx[ri];
                    bool found = false;

                    for (int k = 0; k < static_cast<int>(np.size()); ++k) {
                        if (k == i || npUsed[k]) continue;

                        // Transform existing position under symmetry
                        int symPos[3];
                        symPos[0] = bs.sign[0] * sp[j][0] + bs.offset[0];
                        symPos[1] = bs.sign[1] * sp[j][1] + bs.offset[1];
                        symPos[2] = bs.sign[2] * sp[j][2] + bs.offset[2];

                        // Compare with new position
                        if (symPos[0] == np[k][0] &&
                            symPos[1] == np[k][1] &&
                            symPos[2] == np[k][2]) {
                            npUsed[k] = true;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        posMatch = false;
                        break;
                    }
                    matched++;
                }

                if (!posMatch) continue;

                // Positions match — now check halfcube directions
                bool dirMatch = true;

                for (int ri = 0; ri < static_cast<int>(remPosIdx.size()); ++ri) {
                    int j = remPosIdx[ri];

                    // Find first halfcube in existing figure j
                    int halfcubeDir[3] = {0, 0, 0};
                    bool foundHC = false;
                    for (int c = 0; c < s[j].n_cubes && !foundHC; ++c) {
                        if (s[j].cubes[c].c.isHalf()) {
                            const int *dv = s[j].cubes[c].c.getVector();
                            halfcubeDir[0] = dv[0];
                            halfcubeDir[1] = dv[1];
                            halfcubeDir[2] = dv[2];
                            foundHC = true;
                            break;
                        }
                    }

                    if (!foundHC) {
                        // No halfcube in this figure — skip direction check
                        continue;
                    }

                    // Find corresponding figure in new solution
                    int nk = -1;
                    for (int k = 0; k < static_cast<int>(np.size()); ++k) {
                        if (k == i || npUsed[k]) {
                            nk = k;
                            break;
                        }
                    }

                    // Transform direction under symmetry
                    int symDir[3];
                    symDir[0] = bs.dsign[0] * halfcubeDir[0];
                    symDir[1] = bs.dsign[1] * halfcubeDir[1];
                    symDir[2] = bs.dsign[2] * halfcubeDir[2];

                    // Find first halfcube in new figure nk and compare
                    bool foundHC2 = false;
                    for (int c = 0; c < nf[nk].n_cubes && !foundHC2; ++c) {
                        if (nf[nk].cubes[c].c.isHalf()) {
                            const int *dv = nf[nk].cubes[c].c.getVector();
                            if (dv[0] != symDir[0] || dv[1] != symDir[1] || dv[2] != symDir[2]) {
                                dirMatch = false;
                            }
                            foundHC2 = true;
                            break;
                        }
                    }

                    if (!foundHC2) {
                        // Existing figure has halfcube, new figure doesn't
                        dirMatch = false;
                    }
                }

                if (posMatch && dirMatch) {
                    return false;  // solutions equivalent under this symmetry
                }
            }
        }
    }

    return true;  // unique
}
```

## Key Differences from Original

| Aspect | Original | New |
|--------|----------|-----|
| Data | Box mask (12 bytes, cell-level) | Figure positions + halfcube dirs |
| Comparison | `cb1.mask == cb2.mask` | Position bijection + direction vector equality |
| Symmetry | `rotateZ()`/`rotateX()` mutate box in-place | Precomputed `BoxSymmetry` arrays, no mutation |
| Allocation | `box` copies, `BOOST_FOREACH` | Stack arrays, no heap allocation |
| Halfcube | Encoded in mask as direction code | Explicit `getVector()` comparison |

## Implementation Order

1. Add `BoxSymmetry` struct and `boxSymmetries[4]` to `box_solver.hpp` (before `#endif`)
2. Rewrite `isUnique` in `box_solution.cpp` (replace lines 79-118)
3. Compile and test: `qmake && make -j$(nproc)`

## Verification Points

- `rotateZ` symmetry: sign={-1,-1,1}, offset={3,2,0}, dsign={-1,-1,1}
  - Verify: {-1,-1,1} * (0,0,0) + {3,2,0} = {3,2,0} ✓
- `rotateX` symmetry: sign={1,-1,-1}, offset={0,2,1}, dsign={1,-1,-1}
  - Verify: {1,-1,-1} * (0,0,0) + {0,2,1} = {0,2,1} ✓
- `rotateZX` symmetry: sign={-1,1,-1}, offset={3,0,1}, dsign={-1,1,-1}
  - Verify: {-1,1,-1} * (0,0,0) + {3,0,1} = {3,0,1} → (3,0,1) ✓
  - Compose: rotateZ(0,0,0)=(3,2,0) → rotateX(3,2,0)=(3,0,1) ✓
- Direction transforms match `box::directionSwapZ` and `box::directionSwapX` lookup tables
- No permutation of figure order (solver places figures in fixed order)
