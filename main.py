
import sys

from receive_audio import record_from_esp32
from speech_to_text import transcribe_audio
from command_classifier import classify_command
from command_mapper import map_command
from media_controller import execute_action


def main():
    if len(sys.argv) != 2:
        print("Usage: python main.py COM3")
        return

    port = sys.argv[1]

    try:
        print("\n========== VOICE ASSISTANT ==========\n")

        # Stage 1: Record audio from ESP32
        print("[1] Recording audio from ESP32...")
        audio_file = record_from_esp32(port)

        # Stage 2: Transcribe audio
        print("\n[2] Transcribing audio...")
        transcript = transcribe_audio(audio_file)

        print("Recognized speech:", transcript)

        # Stage 3: Classify command
        print("\n[3] Classifying command...")
        intent = classify_command(transcript)

        print("Detected intent:", intent)

        # Stage 4: Map command
        print("\n[4] Mapping multimedia action...")
        action = map_command(intent)

        print("Mapped action:", action)

        print("\n========== FINAL RESULT ==========")

        if action == "unknown":
            print("Command not recognized.")
        else:
            execute_action(action)
            print("Command successfully executed!")

    except Exception as error:
        print("\nPipeline failed:", error)


if __name__ == "__main__":
    main()
