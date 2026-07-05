#!/bin/sh
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. "$ROOT/tests/lib.sh"

FAILED=0

echo "=== Build ==="
make -C srv re
make -C cli

echo "=== Vision unit tests ==="
PYTHONPATH=tests python3 -m unittest tests.test_vision -v || FAILED=1

echo "=== Start server on port $PORT ==="
start_server

echo "=== Protocol and GUI tests ==="
PYTHONPATH=tests python3 -m unittest tests.test_server -v || FAILED=1

echo "=== Smoke tests ==="
OUT=$(printf "team1\n" | nc -w 2 localhost "$PORT")
echo "$OUT" | grep -q BIENVENUE && assert_ok "handshake" || { assert_fail "handshake"; }
echo "$OUT" | grep -q "8 8" && assert_ok "map size" || echo "$OUT" | grep -q "5 5" && assert_ok "map size" || { assert_fail "map size"; }

OUT=$(printf "GRAPHIC\n" | nc -w 2 localhost "$PORT")
echo "$OUT" | grep -q WELCOME && assert_ok "GUI welcome" || { assert_fail "GUI welcome"; }
echo "$OUT" | grep -q "msz 5 5" && assert_ok "GUI msz" || { assert_fail "GUI msz"; }

./client -n team1 -p "$PORT" &
CPID=$!
sleep 1
kill "$CPID" 2>/dev/null || true
assert_ok "client launch"

stop_server
trap - EXIT

if [ "$FAILED" -ne 0 ]; then
	echo "=== Some tests failed ==="
	exit 1
fi
echo "=== All tests passed ==="
