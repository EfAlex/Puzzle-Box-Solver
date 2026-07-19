# box_solver.cpp — Brute Force Iteration Analysis

## Overview

The solver uses a **recursive backtracking** approach to explore all valid placements of puzzle figures (pieces) into a 3D cube grid. The search space is pruned by connectivity checks, collision detection, and upper-bound heuristics.

## Entry Point: `solve()`

```
solve()
  → iterateFigure(0, {})
      → workerIterate(level, currentSolution, connectivityMask)
```

### `solve()` — Top-Level Setup

1. Validates that `m_solution` dimensions match `m_cube` dimensions.
2. Computes the initial **occupancy bitmask** for the empty cube.
3. Calls `iterateFigure(0, {})` to begin placing figures starting from index 0.

---

## `iterateFigure(level, connectivityMask)`

**Purpose:** Iterate through all possible positions and orientations for figure `level`, then recurse deeper.

### Flow

```
iterateFigure(level, connectivityMask):
    figure = m_figures[level]
    emptyCube = m_cube (copy)

    for each position (x, y, z) in cube:
        for each orientation (axis, angle):
            if canPlace(figure, pos, orientation):
                placed = place(figure, pos, orientation)
                if level+1 == m_figures.size():
                    record solution
                else:
                    nextMask = computeConnectivityMask(placed)
                    iterateFigure(level+1, nextMask)
                unplace(figure, pos, orientation)
```

### Key Observations

- **Position loop:** Brute-force iterates every cell `(x, y, z)` in the cube as a potential anchor point for the current figure.
- **Orientation loop:** Iterates over all valid rotation combinations (axis × angle pairs) for the figure.
- **Copy cost:** `QVector<cube> emptyCube = m_cube;` copies the entire cube state on every call — expensive for deep recursion.
- **No ordering heuristic:** Positions and orientations are tried in raw iteration order, not sorted by any heuristic (e.g., most constrained first).

---

## `workerIterate(level, currentSolution, connectivityMask)`

**Purpose:** The core recursive worker that drives the backtracking search. Called both directly from `iterateFigure` and recursively from itself.

### Flow

```
workerIterate(level, currentSolution, connectivityMask):
    if level == m_figures.size():
        record solution
        return

    figure = m_figures[level]

    for each position (x, y, z):
        for each orientation (axis, angle):
            if !canPlace(figure, pos, orientation, connectivityMask):
                continue

            placed = place(figure, pos, orientation)

            // Pruning: check remaining figures still solvable
            if !isSolvable(level+1, placed):
                unplace
                continue

            // Compute new connectivity mask for next figure
            nextMask = computeConnectivityMask(placed)

            // Recurse
            if level+1 < m_figures.size():
                workerIterate(level+1, currentSolution, nextMask)
            else:
                record solution

            unplace
```

### Key Observations

- **Connectivity mask:** Passed as a parameter, computed incrementally. This is an optimization to avoid recomputing which cells are reachable.
- **`isSolvable` pruning:** Before recursing, checks whether remaining unplaced figures can still fit. This is the **most critical pruning heuristic** — it uses upper bounds on achievable volume per remaining figure.
- **`canPlace` uses connectivityMask:** Only considers positions reachable from the growing connected component, reducing the search space.
- **`place`/`unplace` modify `m_cube` in-place:** The cube state is mutated and restored, avoiding deep copies during recursion.

---

## `canPlace()` — Collision & Connectivity Check

```
canPlace(figure, position, orientation, connectivityMask):
    rotated = rotate(figure.cells, orientation)
    translated = translate(rotated, position)

    for each cell in translated:
        if cell is out of bounds: return false
        if m_cube[cell] is occupied: return false
        if cell is not in connectivityMask: return false

    return true
```

**Key bottleneck:** Called for every (position, orientation) pair. Does full bounds check, collision check, and connectivity check per cell of the figure.

---

## `computeConnectivityMask()` — Flood Fill

```
computeConnectivityMask(placedCells):
    mask = empty bitmask
    queue = [first placed cell]
    mask[first] = true
    while queue not empty:
        cell = pop(queue)
        for each neighbor of cell:
            if not in mask and not occupied:
                add to mask, push to queue
    return mask
```

Called after every successful placement to compute which cells remain reachable for future figures. **Expensive flood fill** on every recursion level.

---

## Search Space Magnitude

For a cube of size N with F figures:

```
Search space ≈ Π(level=0..F-1) [ N³ × orientations(figure[level]) ]
```

Where:
- `N³` = number of possible anchor positions per figure
- `orientations(figure)` = number of valid rotation combinations for that figure
- Pruning via `canPlace` and `isSolvable` reduces this dramatically in practice

---

## Data Structures

| Structure | Type | Purpose |
|-----------|------|---------|
| `m_cube` | `QVector<cube>` | 3D occupancy grid; `cube` = `QVector<int>` of cell values |
| `m_figures` | `QVector<Figure*>` | List of puzzle pieces to place |
| `m_solutions` | `QVector<QVector<cube>>` | Collected valid solutions |
| `connectivityMask` | `QVector<int>` | Reachable cells bitmask |
| `upperBound` | `int` | Precomputed max volume for each figure |

---

## Critical Path Summary

```
iterateFigure / workerIterate
  → canPlace (per cell: bounds + collision + connectivity)
  → place / unplace (mutate m_cube)
  → computeConnectivityMask (flood fill)
  → isSolvable (upper bound check)
```

The **innermost loop** (per position × orientation) does:
1. `canPlace` — O(|figure cells|) per call
2. On success: `place` O(|figure cells|)
3. `computeConnectivityMask` — O(N³) flood fill
4. `isSolvable` — O(remaining figures)
5. Recursive call
6. `unplace` O(|figure cells|)

The flood fill after every placement is the most expensive per-success operation.
