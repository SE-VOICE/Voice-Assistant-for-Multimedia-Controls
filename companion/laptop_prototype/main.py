"""Opt-in laptop experiment. No live control without --execute."""
import argparse
import tempfile
from pathlib import Path
from command_classifier import classify_command
from command_mapper import map_command
from media_controller import execute_action

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port")
    parser.add_argument("--model", help="Existing local Faster-Whisper model directory")
    parser.add_argument("--text", help="Test classification without audio/model dependencies")
    parser.add_argument("--execute", action="store_true", help="Send a real Windows media command")
    args = parser.parse_args(argv)
    if args.text is None and not (args.port and args.model):
        parser.error("Use --text, or both --port and --model")
    if args.text is not None:
        transcript = args.text
    else:
        from receive_audio import record_from_esp32
        from speech_to_text import transcribe_audio
        with tempfile.TemporaryDirectory(prefix="vamc-audio-") as directory:
            audio = record_from_esp32(args.port, Path(directory) / "session.wav")
            transcript = transcribe_audio(audio, args.model)
        # Temporary audio is removed on success and on recognition exceptions.
    intent = classify_command(transcript)
    action = map_command(intent)
    print("Command ID:", intent)
    if action == "unknown":
        return 1
    if args.execute:
        execute_action(action)
        print("Dispatch acknowledged; observe the player to verify execution.")
    else:
        print("Dry run; no media action dispatched.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
