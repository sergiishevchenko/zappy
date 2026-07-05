import time
import unittest

from helpers import (
    connect_player,
    connect_gui,
    send_cmd,
    send_gui,
    connect_raw,
    readline,
    cmd_wait,
    MOVE_TIME,
)

class TestPlayerProtocol(unittest.TestCase):
    def test_full_session(self):
        sock, welcome, slots, dims = connect_player("team1")
        try:
            self.assertEqual(welcome, "BIENVENUE")
            self.assertTrue(slots.isdigit())
            self.assertEqual(len(dims.split()), 2)
            inv = send_cmd(sock, "inventaire")
            self.assertIn("nourriture", inv)
            voir = send_cmd(sock, "voir")
            self.assertNotEqual(voir, "mort")
            self.assertIn("[", voir)
            self.assertIn("]", voir)
            self.assertEqual(send_cmd(sock, "avance"), "ok")
            self.assertEqual(send_cmd(sock, "connect_nbr"), slots)
            self.assertEqual(send_cmd(sock, "sauter"), "ko")
            self.assertEqual(send_cmd(sock, "prend thystame"), "ko")
        finally:
            sock.close()

    def test_unknown_team_rejected(self):
        sock = connect_raw()
        try:
            readline(sock)
            sock.sendall(b"unknown_team\n")
            self.assertEqual(readline(sock), "ko")
        finally:
            sock.close()

    def test_command_queue(self):
        sock, _, _, _ = connect_player()
        try:
            for _ in range(5):
                sock.sendall(b"avance\n")
            time.sleep(cmd_wait("avance") * 5 + 0.1)
            oks = sum(1 for _ in range(5) if readline(sock) == "ok")
            self.assertGreaterEqual(oks, 3)
        finally:
            sock.close()

class TestGuiProtocol(unittest.TestCase):
    def test_full_gui_session(self):
        psock, _, _, _ = connect_player("team2")
        gsock, welcome, lines = connect_gui()
        try:
            self.assertEqual(welcome, "WELCOME")
            text = "\n".join(lines)
            self.assertIn("msz 5 5", text)
            self.assertIn("sgt ", text)
            self.assertIn("pnw #", text)
            self.assertTrue(any(l.startswith("tna ") for l in lines))
            self.assertTrue(any(l.startswith("bct ") for l in lines))
            more = send_gui(gsock, "mgt")
            self.assertTrue(any(l.startswith("mgt ") for l in more))
            more = send_gui(gsock, "sst 500")
            self.assertTrue(any("sst 500" in l for l in more))
            more = send_gui(gsock, "nope")
            self.assertIn("suc", more)
            more = send_gui(gsock, "ppo #9999")
            self.assertIn("sbp", more)
        finally:
            psock.close()
            gsock.close()

if __name__ == "__main__":
    unittest.main()
