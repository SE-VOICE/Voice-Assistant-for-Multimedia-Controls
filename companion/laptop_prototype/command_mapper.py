
ACTION_MAP = {
    "PAUSE": "media_pause",
    "PLAY": "media_play",
    "NEXT": "media_next_track",
    "PREVIOUS": "media_previous_track",
    "RESUME": "media_play",
    "VOLUME_UP": "volume_increase",
    "VOLUME_DOWN": "volume_decrease"
}


def map_command(intent):
    return ACTION_MAP.get(intent, "unknown")
