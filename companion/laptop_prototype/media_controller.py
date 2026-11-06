"""Experimental Windows foreground-app control, not ESP32 BLE HID."""
import ctypes
import sys

# Microsoft WM_APPCOMMAND definitions. Discrete PLAY/PAUSE avoid a toggle.
APP_COMMANDS = {
    "media_play": 46, "media_pause": 47, "media_next_track": 11,
    "media_previous_track": 12, "volume_increase": 10, "volume_decrease": 9,
}

def execute_action(action, sender=None):
    if action not in APP_COMMANDS:
        raise ValueError("Unsupported action")
    if sender is not None:
        return sender(APP_COMMANDS[action])
    if sys.platform != "win32":
        raise RuntimeError("Live media execution requires Windows; use --dry-run elsewhere")
    from ctypes import wintypes
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    user32.GetForegroundWindow.restype = wintypes.HWND
    send = user32.SendMessageTimeoutW
    send.argtypes = [wintypes.HWND, wintypes.UINT, ctypes.c_size_t,
                     ctypes.c_ssize_t, wintypes.UINT, wintypes.UINT,
                     ctypes.POINTER(ctypes.c_size_t)]
    send.restype = ctypes.c_ssize_t
    window = user32.GetForegroundWindow()
    if not window:
        raise RuntimeError("No foreground media application")
    result = ctypes.c_size_t()
    if not send(window, 0x0319, window, APP_COMMANDS[action] << 16,
                0x0002, 1000, ctypes.byref(result)):
        raise RuntimeError("Media dispatch failed or timed out")
    if result.value == 0:
        raise RuntimeError("Foreground application did not acknowledge the media command")
    return True  # Application acknowledgement is not observed playback evidence.
