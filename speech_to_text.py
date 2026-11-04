
from faster_whisper import WhisperModel

print("Loading Faster-Whisper model...")

model = WhisperModel(
    "base",
    device="cpu",
    compute_type="int8"
)


def transcribe_audio(audio_file):
    segments, _ = model.transcribe(
        str(audio_file),
        beam_size=1
    )

    transcript = " ".join(
        segment.text.strip()
        for segment in segments
    )

    return transcript.strip()
