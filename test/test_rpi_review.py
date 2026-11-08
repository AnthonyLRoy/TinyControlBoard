import sys
import types
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts" / "rpi" / "home" / "antho"))
try:
    import requests  # noqa: F401
except ImportError:
    sys.modules["requests"] = types.ModuleType("requests")

import panel_control
import playlist_manager
import mpd_client


class FakeSocket:
    def __init__(self, chunks):
        self.chunks = iter(chunks)

    def settimeout(self, timeout):
        pass

    def recv(self, size):
        return next(self.chunks, b"")

    def sendall(self, data):
        pass

    def close(self):
        pass


class PanelTests(unittest.TestCase):
    def test_closed_connection_raises(self):
        with self.assertRaises(ConnectionError):
            panel_control._read_ws_text_frame(FakeSocket([b""]))

    def test_fragmented_header_and_payload(self):
        sock = FakeSocket([b"\x81", b"\x7e", b"\x00", b"\x03", b"a", b"bc"])
        self.assertEqual(b"abc", panel_control._read_ws_text_frame(sock))


class PlaylistLoadTests(unittest.TestCase):
    def test_failed_load_does_not_remove_current_queue(self):
        commands = []

        def command(value):
            commands.append(value)
            if value == "status":
                return ["playlistlength: 2"]
            raise RuntimeError("load failed")

        with patch.object(playlist_manager, "mpd_command", side_effect=command), \
             patch.object(playlist_manager, "_send_playlist_result") as result:
            playlist_manager.handle_playlist_load("missing")
        self.assertEqual(["status", 'load "missing"'], commands)
        result.assert_called_once_with(False, "load failed")

    def test_successful_load_removes_only_previous_queue(self):
        commands = []

        def command(value):
            commands.append(value)
            return ["playlistlength: 2"] if value == "status" else []

        with patch.object(playlist_manager, "mpd_command", side_effect=command), \
             patch.object(playlist_manager, "_send_playlist_result") as result:
            playlist_manager.handle_playlist_load("saved")
        self.assertEqual(["status", 'load "saved"', "stop", "delete 0:2"], commands)
        result.assert_called_once_with(True, "saved")


class MpdCommandTests(unittest.TestCase):
    def test_eof_during_load_keeps_existing_queue(self):
        status = FakeSocket([b"OK MPD 0.23\n", b"playlistlength: 2\nOK\n"])
        load = FakeSocket([b"OK MPD 0.23\n", b""])
        with patch.object(mpd_client.socket, "create_connection", side_effect=[status, load]) as connect, \
             patch.object(playlist_manager, "_send_playlist_result") as result:
            playlist_manager.handle_playlist_load("saved")
        self.assertEqual(2, connect.call_count)
        result.assert_called_once_with(False, "MPD connection closed before command completed")

    def test_ack_after_response_lines_raises(self):
        sock = FakeSocket([b"OK MPD 0.23\n", b"file: one\nACK [5@0] {load} failed\n"])
        with patch.object(mpd_client.socket, "create_connection", return_value=sock), \
             self.assertRaisesRegex(RuntimeError, r"ACK \[5@0\]"):
            mpd_client.mpd_command('load "saved"')

    def test_ack_inside_value_is_not_a_terminator(self):
        sock = FakeSocket([b"OK MPD 0.23\n", b"Title: ACK record\n", b"OK\n"])
        with patch.object(mpd_client.socket, "create_connection", return_value=sock):
            self.assertEqual(["Title: ACK record"], mpd_client.mpd_command("currentsong"))


if __name__ == "__main__":
    unittest.main()
