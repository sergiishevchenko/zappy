import random
import time
from vision import (
    find_resource, path_to_index, count_players,
    turn_commands,
)

INCANT = {
    1: {"players": 1, "stones": {"linemate": 1}},
    2: {"players": 2, "stones": {"linemate": 1, "deraumere": 1, "sibur": 1}},
    3: {"players": 2, "stones": {"linemate": 2, "sibur": 1, "phiras": 2}},
    4: {"players": 4, "stones": {"linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1}},
    5: {"players": 4, "stones": {"linemate": 1, "deraumere": 2, "sibur": 1, "mendiane": 3}},
    6: {"players": 6, "stones": {"linemate": 1, "deraumere": 2, "sibur": 3, "phiras": 1}},
    7: {"players": 6, "stones": {"linemate": 2, "deraumere": 2, "sibur": 2,
                                  "mendiane": 2, "phiras": 2, "thystame": 1}},
}

class Strategy:
    def __init__(self, client):
        self.c = client
        self.role = random.randint(0, 2)
        self.incanting = False
        self.last_incant = 0
        self.sound_target = None

    def run(self):
        while self.c.alive:
            try:
                self.tick()
            except Exception:
                time.sleep(0.1)
        self.c.close()

    def tick(self):
        self._handle_messages()
        if self.incanting:
            if time.time() - self.last_incant > 32:
                self.c.level += 1
                self.incanting = False
            time.sleep(0.05)
            return
        if self.sound_target is not None:
            self._follow_sound()
            return
        inv = self.c.inventaire()
        if not inv:
            return
        food = inv.get("nourriture", 0)
        if food < 10:
            self._survive()
            return
        slots = self.c.connect_nbr()
        if slots > 0 and food > 25 and self.role == 0:
            self.c.queue("fork")
            self.c.queue("broadcast FORK")
        lvl = self.c.level
        if lvl >= 8:
            self.c.queue("voir")
            return
        if self._try_incant(inv, lvl):
            return
        self._gather_for_level(inv, lvl)

    def _handle_messages(self):
        with self.c.lock:
            msgs = [r for r in self.c.responses if r.startswith("message ")]
            self.c.responses = [r for r in self.c.responses if not r.startswith("message ")]
            for r in self.c.responses:
                if r.startswith("elevation"):
                    self.incanting = True
                    self.last_incant = time.time()
        for m in msgs:
            parts = m[8:].split(",", 1)
            if len(parts) < 2:
                continue
            try:
                k = int(parts[0])
            except ValueError:
                continue
            text = parts[1]
            if text.startswith("COME") or text.startswith("INCANT"):
                self.sound_target = k
            elif text.startswith("FORK") and self.role > 0:
                self.sound_target = k

    def _follow_sound(self):
        k = self.sound_target
        self.sound_target = None
        for cmd in turn_commands(k):
            self.c.queue(cmd)
        for _ in range(4):
            self.c.queue("avance")
        if self.role > 0:
            self.c.queue("incantation")
            self.last_incant = time.time()

    def _survive(self):
        tiles = self.c.voir()
        if not tiles:
            self.c.queue("avance")
            return
        idx = find_resource(tiles, "nourriture")
        if idx >= 0:
            for cmd in path_to_index(idx):
                self.c.queue(cmd)
            self.c.queue("prend nourriture")
            return
        for cmd in ["avance", "avance", "droite"]:
            self.c.queue(cmd)

    def _gather_for_level(self, inv, lvl):
        if lvl >= 8 or lvl not in INCANT:
            self.c.queue("avance")
            return
        req = INCANT[lvl]
        if self.role == 0 and req["players"] > 1:
            self.c.queue("broadcast COME")
        for stone, need in req["stones"].items():
            if inv.get(stone, 0) < need:
                self._gather(stone)
                return
        for stone, need in req["stones"].items():
            for _ in range(need):
                self.c.queue(f"pose {stone}")
        if req["players"] == 1:
            self.c.queue("incantation")
            self.last_incant = time.time()
        elif self.role == 0:
            self.c.queue("broadcast INCANT")
            self.c.queue("incantation")
            self.last_incant = time.time()

    def _try_incant(self, inv, lvl):
        if lvl >= 8 or lvl not in INCANT:
            return False
        req = INCANT[lvl]
        for stone, need in req["stones"].items():
            if inv.get(stone, 0) < need:
                return False
        tiles = self.c.voir()
        if not tiles:
            return False
        if count_players(tiles) < req["players"] - 1:
            return False
        for stone, need in req["stones"].items():
            for _ in range(need):
                self.c.queue(f"pose {stone}")
        self.c.queue("incantation")
        self.last_incant = time.time()
        if self.role == 0:
            self.c.queue("broadcast INCANT")
        return True

    def _gather(self, resource):
        tiles = self.c.voir()
        if not tiles:
            self.c.queue("avance")
            return
        idx = find_resource(tiles, resource)
        if idx < 0:
            for cmd in ["avance", "voir", "droite"]:
                self.c.queue(cmd)
            return
        for cmd in path_to_index(idx):
            self.c.queue(cmd)
        self.c.queue(f"prend {resource}")
