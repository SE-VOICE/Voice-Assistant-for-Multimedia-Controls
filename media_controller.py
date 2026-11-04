
import pyautogui
import time

def execute_action(action):

    if action == "media_play":
        pyautogui.press("playpause")

    elif action == "media_pause":
        pyautogui.press("playpause")

    elif action == "media_next_track":
        pyautogui.press("nexttrack")

    elif action == "media_previous_track":
        pyautogui.press("prevtrack")

    elif action == "audio_mute":
        pyautogui.press("volumemute")

    elif action == "volume_increase":
        pyautogui.press("volumeup", presses=3)

    elif action == "volume_decrease":
        pyautogui.press("volumedown", presses=3)

    else:
        print("Unknown action:", action)
        return

    print("Executed:", action)
