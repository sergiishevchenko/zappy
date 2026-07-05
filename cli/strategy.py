import random
import time

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
        self.role = random.randint(0, 5)
        self.rally = None
        self.incanting = False
        self.last_incant = 0

    def run(self):
        while self.c.alive:
            try:
                self.tick()
            except Exception:
                time.sleep(0.2)
        self.c.close()

    def tick(self):
        self._handle_messages()
        if self.incanting:
            if time.time() - self.last_incant > 35:
                self.c.level += 1
                self.incanting = False
            time.sleep(0.5)
            return
        inv = self.c.inventaire()
        if not inv:
            time.sleep(0.1)
            return
        food = inv.get("nourriture", 0)
        if food < 8:
            self._survive()
            return
        slots = self.c.connect_nbr()
        if slots > 0 and food > 20 and self.role == 0:
            self.c.queue("fork")
        lvl = self.c.level
        if lvl >= 8:
            self.c.queue("voir")
            time.sleep(0.5)
            return
        if self._try_incant(inv, lvl):
            return
        self._gather_for_level(inv, lvl)

    def _handle_messages(self):
        with self.c.lock:
            msgs = [r for r in self.c.responses if r.startswith("message ")]
            self.c.responses = [r for r in self.c.responses if not r.startswith("message ")]
        for m in msgs:
            parts = m[8:].split(",", 1)
            if len(parts) < 2:
                continue
            text = parts[1]
            if text.startswith("RALLY"):
                try:
                    _, xs, ys, ls = text.split()
                    self.rally = (int(xs), int(ys), int(ls))
                except ValueError:
                    pass
            elif text.startswith("INCANT"):
                self.incanting = True
                self.last_incant = time.time()

    def _survive(self):
        tiles = self.c.voir()
        if not tiles:
            self.c.queue(random.choice(["avance", "droite", "gauche"]))
            return
        for i, tile in enumerate(tiles):
            if "nourriture" in tile:
                self._go_to_index(i, len(tiles))
                self.c.queue("prend nourriture")
                return
        for cmd in ["avance", "avance", "droite", "gauche"]:
            self.c.queue(cmd)

    def _gather_for_level(self, inv, lvl):
        if lvl >= 8 or lvl not in INCANT:
            self.c.queue("avance")
            return
        req = INCANT[lvl]
        if self.rally and self.role > 0:
            for _ in range(3):
                self.c.queue("avance")
            if random.random() < 0.4:
                self.c.queue("incantation")
                self.last_incant = time.time()
            return
        if self.role == 0 and req["players"] > 1:
            self.c.queue(f"broadcast RALLY {random.randint(0, self.c.map_w - 1)} "
                         f"{random.randint(0, self.c.map_h - 1)} {lvl}")
        for stone, need in req["stones"].items():
            if inv.get(stone, 0) < need:
                if self._gather(stone):
                    return
                self.c.queue("voir")
                self.c.queue(random.choice(["avance", "droite", "gauche"]))
                return
        for stone, need in req["stones"].items():
            for _ in range(min(need, inv.get(stone, 0))):
                self.c.queue(f"pose {stone}")
        if req["players"] == 1 or self.role == 0:
            self.c.queue("incantation")
            self.last_incant = time.time()
            if self.role == 0:
                self.c.queue(f"broadcast INCANT {lvl}")

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
        on_tile = 0
        for t in tiles:
            if "player" in t:
                on_tile += t.count("player")
        if on_tile >= req["players"] - 1:
            for stone, need in req["stones"].items():
                for _ in range(min(need, inv.get(stone, 0))):
                    self.c.queue(f"pose {stone}")
            self.c.queue("incantation")
            self.last_incant = time.time()
            if self.role == 0:
                self.c.queue(f"broadcast INCANT {lvl}")
            return True
        return False

    def _gather(self, resource):
        tiles = self.c.voir()
        if not tiles:
            return False
        for i, tile in enumerate(tiles):
            if resource in tile:
                self._go_to_index(i, len(tiles))
                self.c.queue(f"prend {resource}")
                return True
        return False

    def _go_to_index(self, idx, total):
        if idx == 0:
            return
        steps = min(idx, 3)
        for _ in range(steps):
            self.c.queue("avance")
        if idx > 3:
            self.c.queue("droite")
            for _ in range(min(idx - 3, 3)):
                self.c.queue("avance")
