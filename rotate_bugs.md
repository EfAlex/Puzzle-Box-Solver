# rotateX / rotateZ Bug Analysis

## Box dimensions
- BOX_W = 4, BOX_H = 3, BOX_D = 2

## rotateZ() — 180° reflection in x-y plane

Mapping: `p1(x,y,z) ↔ p2(3-x, 2-y, z)`

### Loop ranges (correct)
- z: 0..1 (full depth, both layers)
- y: 0..2 (full height of left half)
- x: 0..1 (half width, constrains p1 to one half)

### Why loops are correct
x-loop constrains p1 to x ∈ {0,1} (left half). For each (x,y), p2 = (3-x, 2-y, z) lands in the right half. No overlap, no self-swap, all 12 cells covered in 6 pairs.

### Bug: NONE in loop ranges

---

## rotateX() — 180° reflection in y-z plane

Mapping: `p1(x,y,z) ↔ p2(x, 2-y, 1-z)`

### Loop ranges (buggy)
- z: **0..0** (only z=0) — **BUG: should be 0..1**
- y: 0..2 (full height)
- x: 0..3 (full width, x stays fixed)

### Per-x-layer pairs (y-z plane)
For x=0, z=0 only:

| (y,z) | ↔ | (2-y, 1-z) |
|-------|---|------------|
| (0,0) | ↔ | (2,1) |
| (1,0) | ↔ | (1,1) |
| (2,0) | ↔ | (0,1) |

All 6 cells covered, 3 distinct pairs, no overlap.

### Bug: z-loop range
`z < 1` only processes z=0. The z=1 layer is never a p1 source — those 12 cells (one per x value) are never modified.

**Fix**: `z < BOX_D` (i.e., `z < 2`)

### Common pattern
Both functions use the same swap pattern:
1. Iterate p1 coordinates to cover exactly half the cells
2. Compute p2 as mirror of p1
3. Copy both cells to temp vars
4. Negate relevant direction vector components
5. Swap state + vector into the box

The bug is only in rotateX's z-loop range.
