import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "cli"))

from vision import (
    index_to_row_col,
    path_to_index,
    find_resource,
    count_players,
    turn_commands,
    turns_toward_sound,
)

class TestVision(unittest.TestCase):
    def test_index_row_col_level1(self):
        self.assertEqual(index_to_row_col(0), (0, 0))
        self.assertEqual(index_to_row_col(1), (1, 0))
        self.assertEqual(index_to_row_col(2), (1, 1))
        self.assertEqual(index_to_row_col(3), (1, 2))

    def test_path_to_index_zero(self):
        self.assertEqual(path_to_index(0), ["avance"])

    def test_path_to_index_row1_center(self):
        cmds = path_to_index(2)
        self.assertIn("avance", cmds)
        self.assertEqual(cmds.count("avance"), 2)

    def test_find_resource(self):
        tiles = ["", "nourriture linemate", "sibur"]
        self.assertEqual(find_resource(tiles, "nourriture"), 1)
        self.assertEqual(find_resource(tiles, "thystame"), -1)

    def test_count_players(self):
        tiles = ["player", "player player", ""]
        self.assertEqual(count_players(tiles), 3)

    def test_sound_turns(self):
        self.assertEqual(turns_toward_sound(1), 0)
        self.assertEqual(turns_toward_sound(3), -2)
        self.assertEqual(len(turn_commands(7)), 2)

if __name__ == "__main__":
    unittest.main()
