"""Inject Unicode text / Enter into the focused Windows window via SendInput.

A BLE HID boot keyboard can't type CJK; SendInput with KEYEVENTF_UNICODE can
(full Unicode, surrogate pairs included). This is how the companion lands the
Chinese transcription in whatever window has focus — i.e. Claude desktop.
"""
import ctypes
from ctypes import wintypes

user32 = ctypes.WinDLL("user32", use_last_error=True)

INPUT_KEYBOARD    = 1
KEYEVENTF_KEYUP   = 0x0002
KEYEVENTF_UNICODE = 0x0004
VK_RETURN = 0x0D

ULONG_PTR = ctypes.c_size_t


class MOUSEINPUT(ctypes.Structure):
    _fields_ = (("dx", wintypes.LONG), ("dy", wintypes.LONG),
                ("mouseData", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
                ("time", wintypes.DWORD), ("dwExtraInfo", ULONG_PTR))


class KEYBDINPUT(ctypes.Structure):
    _fields_ = (("wVk", wintypes.WORD), ("wScan", wintypes.WORD),
                ("dwFlags", wintypes.DWORD), ("time", wintypes.DWORD),
                ("dwExtraInfo", ULONG_PTR))


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = (("uMsg", wintypes.DWORD), ("wParamL", wintypes.WORD),
                ("wParamH", wintypes.WORD))


class _INPUTUNION(ctypes.Union):
    _fields_ = (("mi", MOUSEINPUT), ("ki", KEYBDINPUT), ("hi", HARDWAREINPUT))


class INPUT(ctypes.Structure):
    _fields_ = (("type", wintypes.DWORD), ("u", _INPUTUNION))


# Explicit signature so the array pointer isn't truncated on 64-bit Python, and
# so cbSize == sizeof(INPUT) (== 40 on x64) — both else cause WinError 87.
user32.SendInput.argtypes = (wintypes.UINT, ctypes.POINTER(INPUT), ctypes.c_int)
user32.SendInput.restype = wintypes.UINT


def _send(events):
    n = len(events)
    if not n:
        return 0
    arr = (INPUT * n)(*events)
    sent = user32.SendInput(n, arr, ctypes.sizeof(INPUT))
    if sent != n:
        raise ctypes.WinError(ctypes.get_last_error())
    return sent


def _unicode_event(code_unit, keyup):
    flags = KEYEVENTF_UNICODE | (KEYEVENTF_KEYUP if keyup else 0)
    return INPUT(INPUT_KEYBOARD, _INPUTUNION(ki=KEYBDINPUT(0, code_unit, flags, 0, 0)))


def _utf16_units(ch):
    enc = ch.encode("utf-16-le")
    return [enc[i] | (enc[i + 1] << 8) for i in range(0, len(enc), 2)]


def type_unicode(text):
    """Type arbitrary Unicode text into the focused window."""
    events = []
    for ch in text:
        for unit in _utf16_units(ch):
            events.append(_unicode_event(unit, keyup=False))
            events.append(_unicode_event(unit, keyup=True))
    _send(events)


def press_enter():
    down = INPUT(INPUT_KEYBOARD, _INPUTUNION(ki=KEYBDINPUT(VK_RETURN, 0, 0, 0, 0)))
    up = INPUT(INPUT_KEYBOARD, _INPUTUNION(ki=KEYBDINPUT(VK_RETURN, 0, KEYEVENTF_KEYUP, 0, 0)))
    _send([down, up])
