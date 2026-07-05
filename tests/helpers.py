import os
import socket
import time

PORT = int(os.environ.get("ZAPPY_PORT", "4243"))
HOST = os.environ.get("ZAPPY_HOST", "localhost")
FREQ = int(os.environ.get("ZAPPY_FREQ", "100"))
UNIT_SLEEP = float(os.environ.get("ZAPPY_UNIT_SLEEP", "0.02"))

MOVE_TIME = 7
INV_TIME = 1

_leftover = {}

def unit_seconds(units):
    return units / FREQ

def cmd_wait(cmd):
    name = cmd.strip().split()[0]
    if name == "connect_nbr":
        return 0.02
    if name == "inventaire":
        return unit_seconds(INV_TIME) + 0.05
    if name in ("fork", "incantation"):
        return unit_seconds(42) + 0.1
    return unit_seconds(MOVE_TIME) + 0.05

def readline(sock, timeout=5.0):
    key = sock.fileno()
    buf = _leftover.pop(key, b"")
    deadline = time.time() + timeout
    while b"\n" not in buf:
        remaining = deadline - time.time()
        if remaining <= 0:
            if buf:
                _leftover[key] = buf
            raise TimeoutError("timed out waiting for server response")
        sock.settimeout(min(0.1, remaining))
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            continue
        if not chunk:
            raise OSError("connection closed")
        buf += chunk
    line, rest = buf.split(b"\n", 1)
    if rest:
        _leftover[key] = rest
    return line.decode().strip()

def connect_raw():
    sock = socket.create_connection((HOST, PORT), timeout=3)
    sock.settimeout(3)
    return sock

def connect_player(team="team1"):
    sock = connect_raw()
    welcome = readline(sock)
    sock.sendall(f"{team}\n".encode())
    slots = readline(sock)
    dims = readline(sock)
    return sock, welcome, slots, dims

def connect_gui():
    sock = connect_raw()
    readline(sock)
    sock.sendall(b"GRAPHIC\n")
    time.sleep(0.15)
    welcome = readline(sock)
    lines = drain(sock)
    return sock, welcome, lines

def drain(sock, timeout=0.3):
    key = sock.fileno()
    data = _leftover.pop(key, b"")
    sock.settimeout(timeout)
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    sock.settimeout(3)
    lines = [l for l in data.decode().split("\n") if l.strip()]
    if data and not data.endswith(b"\n"):
        tail = data.split(b"\n")[-1]
        if tail.strip():
            _leftover[key] = tail
    return lines

def send_cmd(sock, cmd, wait=None):
    sock.sendall(f"{cmd}\n".encode())
    if wait is None:
        wait = cmd_wait(cmd)
    return readline(sock, timeout=wait + 1.0)

def send_gui(sock, cmd):
    sock.sendall(f"{cmd}\n".encode())
    time.sleep(0.05)
    return drain(sock)
