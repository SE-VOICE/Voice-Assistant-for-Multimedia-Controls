"""Strict phrase matching; never select one command from partial input."""
import re

PHRASES = {
    "play": "PLAY", "play music": "PLAY",
    "pause": "PAUSE", "pause music": "PAUSE",
    "resume": "RESUME", "continue": "RESUME",
    "next": "NEXT", "next track": "NEXT",
    "previous": "PREVIOUS", "previous track": "PREVIOUS",
    "volume up": "VOLUME_UP", "turn it up": "VOLUME_UP",
    "volume down": "VOLUME_DOWN", "turn it down": "VOLUME_DOWN",
}

def classify_command(transcript):
    if not isinstance(transcript, str) or len(transcript) > 128:
        return "UNKNOWN"
    text = " ".join(re.sub(r"[^a-z0-9\s]", " ", transcript.lower()).split())
    return PHRASES.get(text, "UNKNOWN")
