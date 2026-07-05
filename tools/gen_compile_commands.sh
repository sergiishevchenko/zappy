#!/bin/sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RAYLIB=$(pkg-config --cflags raylib 2>/dev/null || echo "-I/usr/local/include")
OUT="$ROOT/compile_commands.json"

python3 - "$ROOT" "$RAYLIB" "$OUT" <<'PY'
import json, os, sys

root, raylib, out = sys.argv[1:4]
raylib_args = raylib.split() if raylib else []

entries = []
for name in ["main", "network", "world", "utils", "commands", "gui", "admin"]:
    entries.append({
        "directory": os.path.join(root, "srv"),
        "file": os.path.join(root, "srv", "src", f"{name}.c"),
        "arguments": [
            "cc", "-Wall", "-Wextra", "-Werror",
            "-I", os.path.join(root, "srv", "include"),
            "-c", os.path.join(root, "srv", "src", f"{name}.c"),
        ],
    })

for name in ["main", "network", "render"]:
    args = [
        "c++", "-Wall", "-Wextra", "-Werror", "-std=c++17",
        "-I", os.path.join(root, "gui", "include"),
    ] + raylib_args + [
        "-c", os.path.join(root, "gui", "src", f"{name}.cpp"),
    ]
    entries.append({
        "directory": os.path.join(root, "gui"),
        "file": os.path.join(root, "gui", "src", f"{name}.cpp"),
        "arguments": args,
    })

with open(out, "w") as f:
    json.dump(entries, f, indent=2)
    f.write("\n")
print(f"Wrote {out} ({len(entries)} entries)")
PY
