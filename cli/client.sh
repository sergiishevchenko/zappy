#!/bin/sh
ROOT="$(cd "$(dirname "$0")" && pwd)"
export PYTHONPATH="$ROOT/cli"
exec python3 "$ROOT/cli/main.py" "$@"
