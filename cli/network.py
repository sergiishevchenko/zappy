import socket
import threading
import time
import subprocess
import sys

class ZappyClient:
    MAX_QUEUE = 10

    def __init__(self, host, port, team):
        self.host = host
        self.port = port
        self.team = team
        self.sock = None
        self.lock = threading.Lock()
        self.responses = []
        self.alive = True
        self.slots = 0
        self.map_w = 0
        self.map_h = 0
        self.level = 1
        self.pending = 0
        self.orient = 1

    def connect(self):
        try:
            self.sock = socket.create_connection((self.host, self.port), timeout=5)
            self.sock.settimeout(0.2)
            line = self._readline()
            if line not in ("WELCOME", "BIENVENUE"):
                return False
            self.send(self.team)
            self.slots = int(self._readline())
            dims = self._readline().split()
            self.map_w, self.map_h = int(dims[0]), int(dims[1])
            threading.Thread(target=self._read_loop, daemon=True).start()
            return True
        except (OSError, ValueError):
            return False

    def _readline(self):
        buf = b""
        while b"\n" not in buf:
            chunk = self.sock.recv(1)
            if not chunk:
                raise OSError("disconnected")
            buf += chunk
        return buf.decode().strip()

    def _read_loop(self):
        buf = ""
        while self.alive:
            try:
                data = self.sock.recv(4096).decode()
                if not data:
                    self.alive = False
                    break
                buf += data
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    line = line.strip()
                    if not line:
                        continue
                    with self.lock:
                        if line == "mort":
                            self.alive = False
                        elif line.startswith("message "):
                            self._on_message(line[8:])
                        elif line.startswith("deplacement "):
                            self.responses.append(line)
                        elif line in ("ok", "ko"):
                            self.responses.append(line)
                            if self.pending > 0:
                                self.pending -= 1
                        elif line.startswith("["):
                            self.responses.append(line)
                        elif line.startswith("elevation"):
                            self.responses.append(line)
                        elif line.startswith("niveau actuel"):
                            self.responses.append(line)
                            try:
                                self.level = int(line.split(":")[1].strip())
                            except (IndexError, ValueError):
                                pass
                        elif line.isdigit():
                            self.responses.append(line)
                            if self.pending > 0:
                                self.pending -= 1
            except socket.timeout:
                continue
            except OSError:
                self.alive = False
                break

    def _on_message(self, payload):
        self.responses.append("message " + payload)
        self._beep()

    def _beep(self):
        try:
            if sys.platform == "darwin":
                subprocess.Popen(["afplay", "/System/Library/Sounds/Ping.aiff"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            else:
                subprocess.Popen(["printf", "\a"], stdout=subprocess.DEVNULL)
        except OSError:
            pass

    def send(self, cmd):
        if not cmd.endswith("\n"):
            cmd += "\n"
        self.sock.sendall(cmd.encode())

    def queue(self, cmd):
        with self.lock:
            if self.pending >= self.MAX_QUEUE:
                return False
            self.pending += 1
        if cmd == "droite":
            self.orient = self.orient % 4 + 1
        elif cmd == "gauche":
            self.orient = self.orient - 1 if self.orient > 1 else 4
        self.send(cmd)
        return True

    def wait_response(self, timeout=15.0):
        deadline = time.time() + timeout
        while time.time() < deadline and self.alive:
            with self.lock:
                if self.responses:
                    return self.responses.pop(0)
            time.sleep(0.01)
        return None

    def cmd(self, command, timeout=15.0):
        if not self.queue(command):
            return None
        return self.wait_response(timeout)

    def inventaire(self):
        r = self.cmd("inventaire", timeout=5)
        inv = {}
        if r and r.startswith("["):
            for part in r.strip("[]").split(","):
                part = part.strip()
                if " " in part:
                    k, v = part.rsplit(" ", 1)
                    inv[k.strip()] = int(v)
        return inv

    def voir(self):
        r = self.cmd("voir", timeout=5)
        if not r or not r.startswith("["):
            return []
        text = r.strip("[]")
        if not text:
            return []
        return [t.strip() for t in text.split(",")]

    def connect_nbr(self):
        r = self.cmd("connect_nbr", timeout=2)
        try:
            return int(r) if r else 0
        except ValueError:
            return 0

    def close(self):
        self.alive = False
        if self.sock:
            self.sock.close()
