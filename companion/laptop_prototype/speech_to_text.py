"""Load a pre-downloaded local model only; no implicit model downloads."""

def transcribe_audio(audio_file, model_path):
    from pathlib import Path
    if not Path(model_path).is_dir():
        raise ValueError("Supply an existing local Faster-Whisper model directory")
    from faster_whisper import WhisperModel
    model = WhisperModel(str(model_path), device="cpu", compute_type="int8",
                         local_files_only=True)
    segments, _ = model.transcribe(str(audio_file), beam_size=1, language="en")
    return " ".join(segment.text.strip() for segment in segments).strip()
