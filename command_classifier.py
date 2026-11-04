
import re

COMMANDS = {
    "stop": "PAUSE",
    "play": "PLAY",
    "next": "NEXT",
    "previous": "PREVIOUS",
    "mute": "MUTE",
    "increased": "VOLUME_UP",
    "decreased": "VOLUME_DOWN"
}


def classify_command(transcript):
    # Convert to lowercase and remove punctuation
    text = transcript.lower()
    text = re.sub(r"[^\w\s]", "", text)
    words = text.split()

    for keyword, intent in COMMANDS.items():
        if keyword in words:
            return intent

    return "UNKNOWN"
