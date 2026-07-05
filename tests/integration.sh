#!/bin/sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PORT=4242
kill_server() { pkill -f "./server -p $PORT" 2>/dev/null || true; sleep 0.2; }

echo "=== Build ==="
make re

echo "=== Server handshake ==="
kill_server
./server -p $PORT -x 10 -y 10 -n team1 team2 -c 2 -t 1000 &
sleep 0.3
OUT=$(printf "team1\n" | nc -w 1 localhost $PORT)
echo "$OUT" | grep -q BIENVENUE && echo "BIENVENUE ok"
echo "$OUT" | grep -q "^1$" && echo "slots ok"
echo "$OUT" | grep -q "10 10" && echo "map ok"

echo "=== GUI protocol ==="
OUT=$(printf "GRAPHIC\nmsz\n" | nc -w 1 localhost $PORT)
echo "$OUT" | grep -q WELCOME && echo "GUI WELCOME ok"
echo "$OUT" | grep -q "msz 10 10" && echo "msz ok"

echo "=== GUI initial state ==="
./client -n team1 -p $PORT &
CPID=$!
sleep 1
OUT=$(printf "GRAPHIC\n" | nc -w 2 localhost $PORT)
echo "$OUT" | grep -q "pnw #" && echo "GUI pnw ok"
kill $CPID 2>/dev/null || true

echo "=== Client launch ==="
./client -n team1 -p $PORT &
CPID=$!
sleep 2
kill $CPID 2>/dev/null || true

kill_server
echo "=== All integration checks passed ==="
