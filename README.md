# Zappy

AI tribes on a looping world — gather stones, hatch eggs, race to level 8.

Teams of autonomous players spawn on a toroidal map with limited food and scattered minerals. They explore by sight and sound, stockpile resources, fork new members from eggs, and gather for incantations to climb eight levels. The first team to get six players to level 8 on the same tile wins.

## Components

| Binary   | Language | Role |
|----------|----------|------|
| `server` | C        | Tick-based game server and protocol handler |
| `client` | Python 3 | Autonomous AI player with vision and strategy |
| `gfx`    | C++      | Real-time viewer built with [Raylib](https://www.raylib.com/) |

## Requirements

- **Server:** a C compiler (`cc`), POSIX sockets
- **Client:** Python 3
- **Viewer:** C++17 compiler, Raylib (`pkg-config --libs raylib`)

On Debian/Ubuntu:

```bash
sudo apt install build-essential python3 libraylib-dev pkg-config
```

## Build

From the project root:

```bash
make
```

Individual targets:

```bash
make server   # build server only
make client   # build client only
make gfx      # build viewer only
make re       # full rebuild
make fclean   # remove all binaries and object files
```

## Quick start

Terminal 1 — start the server:

```bash
./server -p 4242 -x 10 -y 10 -n team1 team2 -c 6 -t 100
```

Terminal 2 — watch the game:

```bash
./gfx -p 4242 -h localhost
```

Terminals 3+ — launch AI clients (one per slot):

```bash
./client -n team1 -p 4242 -h localhost
./client -n team2 -p 4242 -h localhost
```

## Usage

### Server

```
./server -p <port> -x <width> -y <height> -n <team> [<team> ...] -c <nb> -t <t>
```

| Flag | Description |
|------|-------------|
| `-p` | TCP port |
| `-x` | Map width |
| `-y` | Map height |
| `-n` | Team name(s), repeatable |
| `-c` | Max clients per team |
| `-t` | Time unit frequency in Hz (default: 100) |

### Client

```
./client -n <team> -p <port> [-h <hostname>]
```

The client connects as a player, receives map dimensions and available slots, then runs its strategy loop: foraging, broadcasting, following sound cues, and coordinating incantations.

### Viewer

```
./gfx -p <port> -h <hostname>
```

| Key | Action |
|-----|--------|
| Arrow keys | Pan the camera |
| Left click | Inspect a tile |
| Tab | Toggle 2D / 3D view |
| `+` / `-` | Speed up / slow down time units |
| `M` | Toggle background music |

## Project layout

```
zappy/
├── srv/          server source (C)
├── cli/          AI client source (Python)
├── gui/          graphical viewer (C++ / Raylib)
├── tests/        integration checks
└── Makefile      builds server, client, and gfx
```

## Modules

| Module | Type | Status | Details |
|--------|------|--------|---------|
| Server (`server`) | Mandatory | Done | C game server — CLI args, teams, tick loop, win condition |
| Player protocol | Mandatory | Done | Auth, command queue, time units — avance, voir, inventaire, prend, pose, broadcast, incantation, fork, expulse, connect_nbr |
| GUI protocol (server) | Mandatory | Done | GRAPHIC handshake, live updates — msz, bct, pnw, plv, pex, ppo, pin, enw, edi, eht, egd, smg, seg, pic, pie, sgt, sst |
| World & game logic | Mandatory | Done | Toroidal map, resources, food, eggs, incantations, eject, death |
| AI client (`client`) | Mandatory | Done | Python autonomous player — vision parsing, foraging, fork, incantation, broadcast/sound navigation |
| Graphical viewer (`gfx`) | Bonus | Done | Raylib viewer — live map, players, eggs, resources, incantations |
| Viewer extras | Bonus | Done | 2D/3D toggle, tile inspect, broadcast waves, server messages, speed control, background music |
| Administration panel | Bonus | Done | Server stdin — `freq`, `status`, `msg`, `help` |
| Automated tests | Bonus | Done | `tests/run.sh` — vision unit tests, protocol/GUI checks, client smoke test |

## Tests

With `nc` available:

```bash
./tests/integration.sh
```

The script rebuilds the project, verifies the player and GUI handshake, spawns a client, and checks basic protocol responses.

## License

42 Lausanne — educational project.
