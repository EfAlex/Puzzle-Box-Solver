#!/usr/bin/env bash
set -euo pipefail

OUTDIR="perf-output"
DURATION="${1:-30}"  # seconds
APP="${2:-./glboxsolver}"

mkdir -p "$OUTDIR"

# Ensure FlameGraph scripts are on PATH
if [ -d "/home/efalex/git/FlameGraph" ]; then
    export PATH="/home/efalex/git/FlameGraph:$PATH"
fi

echo "=============================================="
echo "  Puzzle Box Solver — Performance Profiling"
echo "=============================================="
echo "  App:     $APP"
echo "  Duration: ${DURATION}s (or until solver finishes)"
echo "  Output:   $OUTDIR/"
echo "=============================================="
echo ""

# Check for perf
if ! command -v perf &>/dev/null; then
    echo "ERROR: 'perf' not found. Install with:"
    echo "  sudo apt install linux-tools-common linux-tools-generic"
    exit 1
fi

# Check for flame graph tools
HAS_FLAMEGRAPH=false
if command -v stackcollapse-perf.pl &>/dev/null && command -v flamegraph.pl &>/dev/null; then
    HAS_FLAMEGRAPH=true
else
    echo "  TIP: Install flame graph for visual profiling:"
    echo "    git clone https://github.com/brendangregg/FlameGraph.git /home/efalex/git/FlameGraph"
    echo "    export PATH=\"/home/efalex/git/FlameGraph:\$PATH\""
    echo ""
fi

echo "[$(date)] Starting perf record (skip first 2s of init)..."
# -D 2000- skips first 2s of startup (OpenGL init, figure setup) — essential for VMs
# -e cpu-clock avoids PMU events that fail in VMs without hardware counters
# Frame-pointer call graph (default) — dwarf mode hangs on perf 7.0.6
timeout "${DURATION}s" perf record -e cpu-clock -g -D 2000- -F 1000 -o "$OUTDIR/perf.data" "$APP" 2>&1 || true
echo "[$(date)] Recording complete."
echo ""

SAMPLES=$(wc -l < "$OUTDIR/perf.data")
echo "[$(date)] Generating reports from $SAMPLES raw events..."
echo ""

# perf report --stdio hangs on perf 7.0.6 — use perf script + text tools
# 1. Flat profile — top functions by self time
echo ">> flat.txt — top functions by self time"
perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
    grep -oE '[a-zA-Z_][a-zA-Z0-9_]+\(' | head -c 50000 | \
    sed 's/(//' | sort | uniq -c | sort -rn | head -50 \
    > "$OUTDIR/flat.txt" 2>&1 || true

# 2. Call tree — show full call chains for top hot functions
echo ">> tree.txt — call tree"
perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
    grep -A7 'workerIterate\|canPlace\|addFigure\|iterateFigure' | head -80 \
    > "$OUTDIR/tree.txt" 2>&1 || true

# 3. Hierarchical tree (parent-based)
echo ">> hierarchy.txt — hierarchical by parent"
perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
    grep -B3 -A5 'addFigure\|rotate\|workerIterate\|mat3_mul_vec' | head -100 \
    > "$OUTDIR/hierarchy.txt" 2>&1 || true

# 4. Line-level breakdown
echo ">> byline.txt — sorted by source file + line"
perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
    grep 'glboxsolver' | grep -oE '[a-zA-Z_][a-zA-Z0-9_:]+\(' | \
    sed 's/(//' | sort | uniq -c | sort -rn | head -50 \
    > "$OUTDIR/byline.txt" 2>&1 || true

# 5. Flame graph
if $HAS_FLAMEGRAPH; then
    echo ">> flame.html — interactive flame graph"
    perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
        stackcollapse-perf.pl 2>/dev/null | \
        flamegraph.pl 2>/dev/null > "$OUTDIR/flame.html" || \
        echo "  (flame graph generation had issues)"
else
    echo ">> flame.html — SKIPPED (install FlameGraph: git clone https://github.com/brendangregg/FlameGraph.git)"
fi

# 6. Annotate key functions — line-level breakdown
echo ">> annotate.txt — line-level annotation of key functions"
KEYFUNCS="canPlace addFigure delFigure workerIterate iterateFigure computeConnectivityMask isSolvable placeFigure rotate"
> "$OUTDIR/annotate.txt"
for func in $KEYFUNCS; do
    echo "" >> "$OUTDIR/annotate.txt"
    echo "========================================" >> "$OUTDIR/annotate.txt"
    echo "  $func" >> "$OUTDIR/annotate.txt"
    echo "========================================" >> "$OUTDIR/annotate.txt"
    perf annotate -i "$OUTDIR/perf.data" --symbol="$func" --stdio 2>&1 >> "$OUTDIR/annotate.txt" || \
        echo "  (no annotation data for $func)" >> "$OUTDIR/annotate.txt"
done

# 7. Per-thread breakdown
echo ">> threads.txt — per-thread breakdown"
perf script -i "$OUTDIR/perf.data" 2>/dev/null | \
    awk '/^box_solver/ { tid=$3 } /glboxsolver/ && /glboxsolver\)/ { print tid, $0 }' | \
    head -100 > "$OUTDIR/threads.txt" 2>&1 || true

# 8. Cache performance — perf stat events may not be available in VMs
echo ">> cache.txt — cache performance summary"
perf stat -a -- ./glboxsolver > "$OUTDIR/cache.txt" 2>&1 || \
    echo "  (perf stat events not available in this environment)" > "$OUTDIR/cache.txt"

echo ""
echo "=============================================="
echo "  DONE. Reports in $OUTDIR/"
echo "=============================================="
echo ""
echo "  flat.txt      — top functions by self time (like 'top')"
echo "  tree.txt      — call tree (hierarchical by comm/dso/symbol)"
echo "  hierarchy.txt — hierarchical by parent call"
echo "  byline.txt    — sorted by source file + line number"
echo "  flame.html    — interactive flame graph (open in browser)"
echo "  annotate.txt  — per-line annotation of key functions"
echo "  threads.txt   — per-thread breakdown"
echo "  cache.txt     — cache performance counters"
echo ""
echo "  To compare before/after:"
echo "    perf diff perf-before/data perf-after/data"
echo ""
echo "  To view flame graph:"
echo "    xdg-open $OUTDIR/flame.html"
echo "=============================================="
