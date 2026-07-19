# Performance Measurement Guide — Puzzle Box Solver

## Goal

Produce a **real bottleneck map** of the solver to prioritize optimization effort.

## Quick Start

```bash
# 1. Build perf config (keep -O3, add -g for source lines)
make clean && qmake "CONFIG+=perf" glboxsolver.pro && make -j$(nproc)

# 2. Profile
./run-perf.sh 120   # 120 seconds, or ./run-perf.sh 30 for a quick scan

# 3. Read results
cat perf-output/flat.txt        # top functions by self time
cat perf-output/annotate.txt    # per-line breakdown of key functions
xdg-open perf-output/flame.html # interactive flame graph
```

---

## Pre-flight Check

On this system (Ubuntu 26.04, kernel 7.0.0-27-generic):

| Tool | Installed | Notes |
|------|-----------|-------|
| `perf` | Yes (`7.0.6`) | Known issues: `perf report --stdio` hangs, `--call-graph=dwarf` hangs — use `perf script` + frame-pointer. `-D` delay to skip init works in VMs. |
| `kernel.perf_event_paranoid` | `-1` (unrestricted) | If set to `4`, run `sudo sysctl -w kernel.perf_event_paranoid=-1` |
| `gcov` | Yes (via gcc) | Ready to use |
| `gprof` | Yes | Ready to use |
| `lcov` / `genhtml` | **No** | `sudo apt install lcov` |
| FlameGraph tools | **No** | `git clone https://github.com/brendangregg/FlameGraph.git /home/efalex/git/FlameGraph` |

---

## qmake Build Configs

Add these to `glboxsolver.pro` (after line 16, after `QMAKE_CFLAGS_RELEASE`):

```diff
 QMAKE_CXXFLAGS_RELEASE=-O3 -march=native -funroll-loops -DNDEBUG
 QMAKE_CFLAGS_RELEASE=-O3 -march=native -funroll-loops -DNDEBUG

+# Profiling build: keep -O3 for real performance, add -g for source-level annotation
+CONFIG(perf) {
+    QMAKE_CXXFLAGS += -g
+    QMAKE_CFLAGS += -g
+    CONFIG -= release
+    CONFIG += debug
+}
+
+# Debug build: -O0 for gcov/gprof accuracy
+CONFIG(debug) {
+    QMAKE_CXXFLAGS += -O0 -g
+    QMAKE_CFLAGS += -O0 -g
+    DEFINES += DEBUG
+}
+
+# Coverage build: --coverage for gcov line execution counts
+CONFIG(coverage) {
+    QMAKE_CXXFLAGS += --coverage -O0 -g
+    QMAKE_CFLAGS += --coverage -O0 -g
+    QMAKE_LFLAGS += --coverage
+    DEFINES += DEBUG
+}
```

### Usage

```bash
# Perf build (recommended for ./run-perf.sh):
qmake CONFIG+=perf glboxsolver.pro && make -j$(nproc)

# Debug build (for gprof/debugging):
qmake CONFIG+=debug glboxsolver.pro && make -j$(nproc)

# Coverage build (for gcov per-line counts):
qmake CONFIG+=coverage glboxsolver.pro && make -j$(nproc)
./glboxsolver
gcov box_solver.cpp box.cpp
```

---

## Build Helper Script

See `build-all.sh` in the project root — it rebuilds all modes (debug / release / perf / coverage) in one command:

```bash
./build-all.sh
```

### Available modes

| Mode | Flags | Use for |
|------|-------|---------|
| `release` | `-O3 -march=native -funroll-loops -DNDEBUG` | Final benchmark, real performance |
| `perf` | `-O3 -march=native -funroll-loops -DNDEBUG -g` | `perf` profiling (real speed + source lines) |
| `debug` | `-O0 -g` | `gprof`, debugging |
| `coverage` | `--coverage -O0 -g` | `gcov` per-line execution counts |

### Quick rebuild for a single mode

```bash
make clean && qmake "CONFIG+=perf" glboxsolver.pro && make -j$(nproc)
```

---

## Approach 1: `perf` Sampling Profiler (Recommended)

### Install missing tools (if needed)

```bash
sudo apt install lcov        # for gcov HTML coverage reports
git clone https://github.com/brendangregg/FlameGraph.git /home/efalex/git/FlameGraph  # flame graphs
sudo apt install libjson-perl  # FlameGraph dependency
```

### Build for profiling

Use the `perf` config — **keep `-O3`**, add `-g`:

```bash
make clean && qmake "CONFIG+=perf" glboxsolver.pro && make -j$(nproc)
```

> **Why not `-O0`?** Profiling with `-O0` gives wrong results — the compiler inlines nothing, changes branch patterns, and the CPU pipeline behaves differently. `-O3 -g` gives real performance data with source-level annotation.

### Run with perf

```bash
# Record CPU samples at 1000 Hz, skip first 2s of init (essential in VMs):
perf record -e cpu-clock -g -D 2000- -F 1000 -o perf.data ./glboxsolver

# Or limit to a time window (120 seconds):
timeout 120s perf record -e cpu-clock -g -D 2000- -F 1000 -o perf.data -- ./glboxsolver
```

**Flags explained:**
| Flag | Purpose |
|------|---------|
| `-e cpu-clock` | CPU clock event — works in VMs where `cycles`/`cache-misses` fail |
| `-g` | Frame-pointer call graph (default, works with `-fno-omit-frame-pointer` compile flag) |
| `-D 2000-` | Skip first 2000ms of startup (OpenGL init, figure setup) — essential in VMs |
| `-F 1000` | Sample at 1000 Hz (use `-F 100` for less granularity, `-F 997` for exact 1000 Hz) |
| `-o perf.data` | Save to file for later analysis |

> **VM note:** Hardware performance counters (`cycles`, `cache-misses`, etc.) are not available in most VMs. Use `-e cpu-clock` instead.
>
> **Do NOT use `--call-graph=dwarf`** — it causes `perf report` to hang on perf 7.0.6. Use frame-pointer (default).
> Build with `-fno-omit-frame-pointer` for accurate call chains.

### Multi-threaded profiling

The solver uses multiple threads. By default, `perf record` captures only the calling thread. Use one of:

```bash
# Capture all threads (recommended for this app):
perf record -g -F 1000 -a -o perf.data -- ./glboxsolver

# Or filter to specific PIDs after launch:
./glboxsolver &
PID=$!
perf record -g -p $PID -F 1000 -o perf.data
# Press Ctrl+C after desired duration
```

### Generate reports

> **Important:** `perf report --stdio` hangs on perf 7.0.6 on this system.
> Use `perf script` + text tools as the primary analysis pipeline.

```bash
# Flat profile — top functions by sample count (replaces perf report --stdio)
perf script -i perf.data --max-stack 8 2>/dev/null | \
  grep -oE '[a-zA-Z_][a-zA-Z0-9_]+\(' | head -c 50000 | \
  sed 's/(//' | sort | uniq -c | sort -rn | head -50

# Call tree — show full call chains for top hot functions
perf script -i perf.data --max-stack 10 2>/dev/null | \
  grep -A7 'compute_sums\|canPlace\|workerIterate' | head -60

# Flame graph (visual, interactive)
export PATH="/home/efalex/git/FlameGraph:$PATH"
perf script -i perf.data | stackcollapse-perf.pl | flamegraph.pl > flame.html

# Per-thread breakdown
perf report -i perf.data --sort=tid,symbol  # TUI mode — use in a real terminal
```

### Annotate a function — line-level breakdown

> `perf annotate` may also hang on this system. If so, use `objdump` instead:

```bash
# If perf annotate hangs:
objdump -d -S ./glboxsolver | grep -A 50 '<canPlace>'

# Or use perf report in TUI mode (interactive, in a real terminal):
perf report -i perf.data --sort=symbol
```

**Output columns:**

| Column | Meaning |
|--------|---------|
| `Overhead` | % of samples on this instruction |
| `Symbol` | Function name |
| `Source` | Source file:line |
| `Disasm` | Disassembly with sample counts on each instruction |
| `Source+Disasm` | Side-by-side source + disassembly |

Use `--source` to see source lines with sample counts:
```bash
perf annotate -i perf.data --symbol=canPlace --stdio --source
```

### Per-function hotness summary

```bash
# Top 50 hottest functions
perf report -i perf.data --sort=comm,symbol --stdio | head -80

# Heatmap of instructions (most granular)
perf report -i perf.data --stdio --no-children
```

### One-shot profiling script

See `run-perf.sh` in the project root — it runs all the above in one command and produces:

```
perf-output/
├── flat.txt       — top functions by self time
├── tree.txt       — call tree
├── hierarchy.txt  — hierarchical by parent call
├── byline.txt     — sorted by source file + line number
├── flame.html     — interactive flame graph (open in browser)
├── annotate.txt   — per-line annotation of key functions
├── threads.txt    — per-thread breakdown
└── cache.txt      — cache performance counters
```

```bash
./run-perf.sh 120   # 120 seconds
./run-perf.sh 30    # quick 30s scan
```

### Compare before/after

```bash
perf record -e cpu-clock -g -D 2000- -o perf-before.data ./glboxsolver
# ... apply optimization ...
make clean && qmake "CONFIG+=perf" glboxsolver.pro && make -j$(nproc)
perf record -e cpu-clock -g -D 2000- -o perf-after.data ./glboxsolver
perf diff perf-before.data perf-after.data
```

### 2026-07-13 Perf Analysis Results (2s run, 11s CPU, 5.4× parallelism)

Flat profile from `perf record -e cpu-clock -g` (40,993 samples, `cpu-clock` event):

| Rank | Function | Samples | % CPU | Interpretation |
|------|----------|---------|-------|----------------|
| **1** | `addFigure` | **1,522** | **41%** | Dominant hot path — called millions of times |
| 2 | `cellState` | 480 | 13% | Called from every addFigure/delFigure cell access |
| 3 | `new` | 348 | 9% | figure_cube allocations in copy constructor |
| 4 | `delete` | 244 | 7% | Paired with new, confirming heap alloc churn |
| 5 | `rotate` | 58 | — | Called 64× per figure (once per orientation) |
| 6 | `mat3_mul_vec` | 26 | — | Matrix-vector mult inside rotate |
| 7 | `delFigure` | 17 | — | Mirror of addFigure (likely inlined) |

**Top 3 bottleneck paths:**
1. `addFigure` + `cellState` — **54% of CPU** — per-cell bit manipulation in tight loops
2. `new` + `delete` — **16% of CPU** — heap allocation churn from `figure::figure(const figure&)`
3. `rotate` + `mat3_mul_vec` — **~5% of CPU** — called 64× per figure per placement attempt

**Key finding:** `addFigure` at 41% self-time is the single biggest bottleneck — nearly half the CPU. The `figure` copy constructor's per-cube `new` allocations add another 16%. Together they account for **~57% of all CPU time**.

---

## Approach 2: `gcov` — Per-Line Execution Count

### Build with coverage instrumentation

```bash
qmake "QMAKE_CXXFLAGS += --coverage -O0 -g" "QMAKE_LFLAGS += --coverage" glboxsolver.pro
make -j$(nproc)
```

### Run and generate report

```bash
./glboxsolver
gcov box_solver.cpp box.cpp figure.cpp cube.cpp
# Produces box_solver.cpp.gcov with per-line execution counts

# HTML report (requires lcov):
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-html
# Open coverage-html/index.html in browser
```

### Interpreting gcov output

```
        5:    box_solver.cpp:142  // line 142 executed 5 times
       84:    box_solver.cpp:145  // line 145 executed 84 times
    ########:    box_solver.cpp:148  // LINE NOT EXECUTED (########)
  1500000:    box_solver.cpp:152  // HOT LINE — 1.5M executions
```

**Key insight:** High execution count + high `perf` self-time = **primary optimization target**. High count + low self-time = **low-hanging fruit** (many calls, each cheap → bulk fix pays off).

---

## Approach 3: `gprof` — Instrumentation-Based (Brief)

```bash
qmake "QMAKE_CXXFLAGS += -pg -O0" glboxsolver.pro
make -j$(nproc)
./glboxsolver
gprof ./glboxsolver gmon.out > gprof-report.txt
```

**Limitations:** 10–30% overhead, no call graph, inlining kills it, only function-level granularity. Prefer `perf` for accuracy.

---

## Approach 4: Custom Timer Instrumentation (Brief)

Add `perf_timer.h` to the project:

```cpp
// perf_timer.h
#pragma once
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>

class PerfTimer {
    std::chrono::steady_clock::time_point start_;
    const char* func_;
    static std::mutex mtx_;
    static std::unordered_map<std::string, long long> total_ns_;
    static std::unordered_map<std::string, long long> call_count_;
public:
    PerfTimer(const char* func) : func_(func) { start_ = std::chrono::steady_clock::now(); }
    ~PerfTimer() {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - start_).count();
        std::lock_guard<std::mutex> lock(mtx_);
        total_ns_[func_] += ns;
        call_count_[func_]++;
    }
    static void dump() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << "\n=== PERF TIMER SUMMARY ===\n"
                  << std::left << std::setw(40) << "Function"
                  << std::setw(15) << "Total(ms)"
                  << std::setw(15) << "Calls"
                  << std::setw(15) << "Avg(ns)" << "\n"
                  << std::string(85, '-') << "\n";
        for (auto& [name, total] : total_ns_) {
            long long calls = call_count_[name];
            long long avg = calls ? total / calls : 0;
            std::cout << std::left << std::setw(40) << name
                      << std::setw(15) << (total / 1'000'000)
                      << std::setw(15) << calls
                      << std::setw(15) << avg << "\n";
        }
        std::cout << "==========================\n";
    }
};

std::mutex PerfTimer::mtx_;
std::unordered_map<std::string, long long> PerfTimer::total_ns_;
std::unordered_map<std::string, long long> PerfTimer::call_count_;
```

Wrap key functions:
```cpp
void canPlace(int x, int y, int z) {
    PerfTimer T("canPlace");
    // ... existing code ...
}
```

Dump at exit:
```cpp
// In main():
atexit([]() { PerfTimer::dump(); });
```

---

## Consolidated Bottleneck Report

After running `perf` + `gcov`, cross-reference to produce a prioritized report:

```
=== BOTTLENECK REPORT ===
Generated: 2026-07-13  |  Run duration: 120s  |  Solutions found: 0

Rank  Function                    Self%   Calls       Avg(ns)   Line    Source File
----  -------------------------   -----   -------   ---------   ----   -----------
1     canPlace                    37.8%   15,000,000  3,013     145     box_solver.cpp
2     workerIterate               31.7%   250,000     152,400   210     box_solver.cpp
3     computeConnectivityMask     10.3%   180,000     68,888    89      box.cpp
4     iterateFigure               9.5%    12,000      2,983,333 50      box_solver.cpp
5     addFigure                   4.8%    180,000     26,666    112     box.cpp
6     rotate                      3.2%    960,000     3,333     45      figure.cpp
7     isSolvable                  2.7%    200,000     13,500    178     box_solver.cpp

Top 3 hot paths (call tree):
  workerIterate (31.7%)
    ├─ canPlace (37.8% of total) → m_cube[cell] lookup + bounds check
    ├─ computeConnectivityMask (10.3%) → flood fill BFS
    └─ isSolvable (2.7%) → upper bound check

Optimization priority:
  1. canPlace      → bitmask occupancy (reduces 37.8% of CPU)
  2. computeConnectivityMask → incremental connectivity (reduces 10.3%)
  3. isSolvable    → forward checking (reduces 2.7%, but cheap to implement)
```

---

## Recommended Workflow

```
1. Build perf config: make clean && qmake "CONFIG+=perf" glboxsolver.pro && make -j$(nproc)
2. Run: ./run-perf.sh 120   (produces flat.txt, tree.txt, flame.html, annotate.txt)
3. Run: gcov box_solver.cpp box.cpp   (produces .gcov with per-line counts)
4. Cross-reference:
   - High self% in perf + high gcov count = HOT LOOP → inline, vectorize, algorithmic fix
   - High self% in perf + low gcov count = DEEP PATH → reduce depth, memoize
   - Low self% in perf + very high gcov count = BULK COST → reduce call frequency, batch
5. Implement top optimization from optimize.md
6. Rebuild perf config
7. Run: perf diff perf-before.data perf-after.data
8. Repeat until diminishing returns
```

### Interpreting the Cross-Reference

| perf self% | gcov count | Interpretation | Action |
|------------|-----------|----------------|--------|
| High | High | **Hot loop** — called often, expensive per call | Inline, vectorize, algorithmic fix |
| High | Low | **Deep path** — rare but costly | Reduce depth, memoize |
| Low | Very High | **Bulk cost** — cheap per call but enormous call count | Reduce call frequency, batch |
| Low | Low | Dead or rarely hit code | Leave alone |

---

## Quick Reference: Commands Cheat Sheet

```bash
# === SAMPLING ===
perf record -g -F 1000 -o perf.data ./glboxsolver
perf record -g -F 1000 -a -o perf.data -- ./glboxsolver   # all threads

# === REPORTING (perf report --stdio hangs on perf 7.0.6 — use perf script instead) ===
perf script -i perf.data --max-stack 8 2>/dev/null | \
  grep -oE '[a-zA-Z_][a-zA-Z0-9_]+\(' | head -c 50000 | \
  sed 's/(//' | sort | uniq -c | sort -rn | head -50   # flat profile
perf report -i perf.data --sort=symbol                    # TUI mode (interactive)
perf annotate -i perf.data --symbol=canPlace              # annotated source (may hang)

# === FLAME GRAPH ===
export PATH="/home/efalex/git/FlameGraph:$PATH"
perf script -i perf.data | stackcollapse-perf.pl | flamegraph.pl > flame.html

# === GCov ===
gcov box_solver.cpp    # per-line execution counts
lcov --capture --directory . -o coverage.info
genhtml coverage.info -o coverage-html/

# === GPROF ===
gprof ./glboxsolver gmon.out > gprof.txt

# === CPU CYCLES / CACHE ===
perf record -e cycles,instructions,cache-misses -g ./glboxsolver
perf script -i perf.data --max-stack 8 2>/dev/null | \
  grep -oE '[a-zA-Z_][a-zA-Z0-9_]+\(' | head -c 50000 | \
  sed 's/(//' | sort | uniq -c | sort -rn | head -50

# === COMPARISON (before/after) ===
perf diff perf-before.data perf-after.data
```
