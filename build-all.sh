#!/usr/bin/env bash
set -euo pipefail

PROJ="glboxsolver.pro"
MODES=("debug" "release" "perf" "coverage")

for mode in "${MODES[@]}"; do
    echo "=============================================="
    echo "  Building: $mode"
    echo "=============================================="
    make clean 2>/dev/null || true
    qmake "CONFIG+=$mode" "$PROJ"
    make -j"$(nproc)"
    echo "  [OK] $mode build complete"
    echo ""
done

echo "=============================================="
echo "  All builds complete."
echo "  Targets:"
echo "    ./glboxsolver   — release (default)"
echo "    CONFIG+=debug   — debug (-O0 -g)"
echo "    CONFIG+=perf    — profiling (-O3 -g)"
echo "    CONFIG+=coverage — gcov (--coverage -O0 -g)"
echo "=============================================="
