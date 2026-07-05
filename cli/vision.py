def index_to_row_col(idx):
    row = 0
    start = 0
    while True:
        width = 2 * row + 1
        if idx < start + width:
            return row, idx - start
        start += width
        row += 1

def path_to_index(idx):
    row, col = index_to_row_col(idx)
    cmds = []
    for _ in range(row + 1):
        cmds.append("avance")
    offset = col - row
    if offset > 0:
        cmds.append("droite")
        for _ in range(offset):
            cmds.append("avance")
    elif offset < 0:
        cmds.append("gauche")
        for _ in range(-offset):
            cmds.append("avance")
    return cmds

def find_resource(tiles, resource):
    for i, tile in enumerate(tiles):
        if resource in tile.split():
            return i
    return -1

def count_players(tiles):
    total = 0
    for tile in tiles:
        total += tile.count("player")
    return total

SOUND_TURNS = [0, 0, -1, -2, -3, 4, 3, 2, 1]

def turns_toward_sound(k):
    if 0 <= k < len(SOUND_TURNS):
        return SOUND_TURNS[k]
    return 0

def turn_commands(k):
    turns = turns_toward_sound(k)
    cmds = []
    if turns > 0:
        cmds.extend(["droite"] * turns)
    elif turns < 0:
        cmds.extend(["gauche"] * (-turns))
    return cmds
