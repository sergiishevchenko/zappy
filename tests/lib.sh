#!/bin/sh

PORT=${ZAPPY_PORT:-4243}
FREQ=${ZAPPY_FREQ:-100}
export ZAPPY_PORT=$PORT
export ZAPPY_FREQ=$FREQ
export ZAPPY_UNIT_SLEEP=${ZAPPY_UNIT_SLEEP:-0.02}

kill_server() {
	pkill -f "./server -p $PORT " 2>/dev/null || true
	pkill -f "./server -p $PORT$" 2>/dev/null || true
	sleep 0.2
}

start_server() {
	kill_server
	./server -p "$PORT" -x 5 -y 5 -n team1 team2 -c 6 -t "$FREQ" < /dev/null &
	SERVER_PID=$!
	sleep 0.5
	if ! nc -z localhost "$PORT" 2>/dev/null; then
		echo "Server failed to start on port $PORT"
		exit 1
	fi
}

stop_server() {
	kill -TERM "$SERVER_PID" 2>/dev/null || true
	sleep 0.3
	kill -KILL "$SERVER_PID" 2>/dev/null || true
	kill_server
}

trap stop_server EXIT

assert_ok() {
	printf "  $(printf '\033[1;32mok\033[0m')  %s\n" "$1"
}

assert_fail() {
	printf "  $(printf '\033[1;31mFAIL\033[0m')  %s\n" "$1"
	exit 1
}
