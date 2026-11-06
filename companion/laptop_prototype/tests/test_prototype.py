import contextlib
import io
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from command_classifier import classify_command
from command_mapper import map_command
from media_controller import execute_action
from main import main

class PrototypeTests(unittest.TestCase):
    def test_seven_commands(self):
        for phrase, intent in [("play", "PLAY"), ("pause", "PAUSE"),
                ("resume", "RESUME"), ("next", "NEXT"), ("previous", "PREVIOUS"),
                ("volume up", "VOLUME_UP"), ("volume down", "VOLUME_DOWN")]:
            with self.subTest(phrase=phrase):
                self.assertEqual(classify_command(phrase), intent)
                self.assertNotEqual(map_command(intent), "unknown")

    def test_approved_variants_and_normalization(self):
        self.assertEqual(classify_command(" TURN it UP! "), "VOLUME_UP")
        self.assertEqual(classify_command("continue"), "RESUME")
        self.assertEqual(classify_command("volume-down"), "VOLUME_DOWN")

    def test_ambiguous_and_partial_inputs_rejected(self):
        for phrase in ["pause next", "play previous", "do not play", "play dishwasher", "mute", "stop"]:
            self.assertEqual(classify_command(phrase), "UNKNOWN")

    def test_invalid_inputs_rejected(self):
        for phrase in [None, "", " " * 129, 42]:
            self.assertEqual(classify_command(phrase), "UNKNOWN")

    def test_discrete_play_pause_resume(self):
        sent = []
        for intent in ["PLAY", "PAUSE", "RESUME"]:
            execute_action(map_command(intent), sender=sent.append)
        self.assertEqual(sent, [46, 47, 46])

    def test_invalid_action_sends_nothing(self):
        sender = unittest.mock.Mock()
        with self.assertRaises(ValueError):
            execute_action("unknown", sender)
        sender.assert_not_called()

    def test_non_windows_execution_fails_explicitly(self):
        with patch("media_controller.sys.platform", "darwin"):
            with self.assertRaises(RuntimeError):
                execute_action("media_play")

    def test_cli_defaults_to_dry_run(self):
        with patch("main.execute_action") as send, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(main(["--text", "pause"]), 0)
            send.assert_not_called()

    def test_rejected_input_never_dispatches(self):
        with patch("main.execute_action") as send, contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(main(["--text", "pause next", "--execute"]), 1)
            send.assert_not_called()

    def test_audio_file_cleanup_even_when_transcription_fails(self):
        observed = []
        def record(port, output):
            output.write_bytes(b"fake-test-data")
            observed.append(output)
            return output
        fake_audio = types.SimpleNamespace(record_from_esp32=record)
        def fail(*args):
            raise RuntimeError("injected transcription failure")
        fake_stt = types.SimpleNamespace(transcribe_audio=fail)
        with patch.dict(sys.modules, {"receive_audio": fake_audio, "speech_to_text": fake_stt}):
            with self.assertRaises(RuntimeError):
                main(["--port", "fake", "--model", "fake"])
        self.assertEqual(len(observed), 1)
        self.assertFalse(observed[0].exists())

if __name__ == "__main__":
    unittest.main()
