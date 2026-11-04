
import time
import wave
import struct
from pathlib import Path

import serial

BAUD_RATE = 921600
SAMPLE_RATE = 16000
RECORD_SECONDS = 5

EXPECTED_BYTES = SAMPLE_RATE * RECORD_SECONDS * 2

RECORDINGS_DIR = Path(__file__).resolve().parent / "recordings"
RECORDINGS_DIR.mkdir(exist_ok=True)


def read_exact(connection, count):
    result = bytearray()

    while len(result) < count:
        chunk = connection.read(count - len(result))

        if not chunk:
            raise TimeoutError(
                f"Received {len(result)} of {count} expected bytes."
            )

        result.extend(chunk)

    return bytes(result)


def record_from_esp32(port):
    output = RECORDINGS_DIR / "recording.wav"

    with serial.Serial(
        port=port,
        baudrate=BAUD_RATE,
        timeout=60,
        write_timeout=10
    ) as connection:

        time.sleep(2)
        connection.reset_input_buffer()

        print("Requesting 5-second recording...")

        connection.write(b"r")
        connection.flush()

        header = read_exact(connection, 8)

        if header[:4] != b"AUD0":
            raise RuntimeError(f"Invalid header: {header!r}")

        data_length = struct.unpack("<I", header[4:])[0]

        if data_length != EXPECTED_BYTES:
            raise ValueError(
                f"Unexpected audio size: {data_length} bytes."
            )

        print("Receiving audio...")

        audio_data = read_exact(connection, data_length)
        trailer = read_exact(connection, 4)

        if trailer != b"END0":
            raise RuntimeError(f"Invalid trailer: {trailer!r}")

    with wave.open(str(output), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(audio_data)

    print(f"Recording saved: {output.resolve()}")

    return output
