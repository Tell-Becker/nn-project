#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
spike_src="${SPIKE_SRC:-$HOME/riscv/riscv-isa-sim}"
customext_dir="$spike_src/customext"
customext_make="$customext_dir/customext.mk.in"

if [[ ! -f "$customext_make" || ! -d "$spike_src/build" ]]; then
    printf 'Spike source/build not found at %s\n' "$spike_src" >&2
    printf 'Set SPIKE_SRC to a configured riscv-isa-sim checkout.\n' >&2
    exit 1
fi

cp "$repo_root/src/test_mac/custom_isa_ref.h" "$customext_dir/custom_isa_ref.h"
cp "$repo_root/spike/xqmac8.cc" "$customext_dir/xqmac.cc"

python3 - "$customext_make" "$spike_src/build/customext.mk" <<'PY'
from pathlib import Path
import sys

for raw_path in sys.argv[1:]:
    path = Path(raw_path)
    text = path.read_text()
    if "xqmac.cc" not in text:
        source_list_end = "\tcflush.cc \\\n"
        if source_list_end not in text:
            raise SystemExit(
                f"Cannot safely update {path}: expected source list not found"
            )
        path.write_text(text.replace(
            source_list_end, source_list_end + "\txqmac.cc \\\n", 1
        ))
PY

make -C "$spike_src/build" -j"$(nproc)" libcustomext.so
printf 'Built Xqmac8 Spike extension: %s/build/libcustomext.so\n' "$spike_src"
