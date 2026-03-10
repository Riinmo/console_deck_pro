import math
import os
import platform
import subprocess
import threading
import time
import wave
import shutil
import hashlib
from pathlib import Path
from array import array

try:
    import pygame
    HAS_PYGAME = True
except Exception:
    HAS_PYGAME = False

try:
    # 1.2.2 is the stable non-GStreamer path on Windows.
    from playsound import playsound
    HAS_PLAYSOUND = True
except Exception:
    HAS_PLAYSOUND = False

try:
    import winsound  # Windows fallback for default notes when pygame is unavailable
    HAS_WINSOUND = True
except Exception:
    HAS_WINSOUND = False


def _play_wav_fallback(file_path):
    """Play a WAV file using system player (macOS: afplay, Linux: aplay). No-op on Windows (uses winsound)."""
    if not file_path or not os.path.isfile(file_path):
        return
    try:
        system = platform.system()
        if system == "Darwin":
            subprocess.Popen(["afplay", file_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elif system == "Linux":
            subprocess.Popen(["aplay", "-q", file_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except (FileNotFoundError, OSError):
        pass


class SpecialModuleManager:
    """
    Manages special module behaviors (media deck + piano) outside main loop logic.
    """

    DEFAULT_PIANO_NOTES = {
        "piano_key_1": "C4",
        "piano_key_2": "D4",
        "piano_key_3": "E4",
        "piano_key_4": "F4",
        "piano_key_5": "G4",
        "piano_key_6": "A4",
        "piano_key_7": "B4",
        "piano_black_1": "C#4",
        "piano_black_2": "D#4",
        "piano_black_3": "F#4",
        "piano_black_4": "G#4",
        "piano_black_5": "A#4",
    }

    PIANO_KEY_ORDER = [
        "piano_key_1",
        "piano_key_2",
        "piano_key_3",
        "piano_key_4",
        "piano_key_5",
        "piano_key_6",
        "piano_key_7",
        "piano_black_1",
        "piano_black_2",
        "piano_black_3",
        "piano_black_4",
        "piano_black_5",
    ]

    NOTE_FREQUENCIES = {
        "C4": 261.63,
        "C#4": 277.18,
        "D4": 293.66,
        "D#4": 311.13,
        "E4": 329.63,
        "F4": 349.23,
        "F#4": 369.99,
        "G4": 392.00,
        "G#4": 415.30,
        "A4": 440.00,
        "A#4": 466.16,
        "B4": 493.88,
    }

    def __init__(self, app_dir, event_log=False, action_log=False):
        self.app_dir = app_dir
        self.event_log = event_log
        self.action_log = action_log
        self.lock = threading.RLock()

        self.media_cfg = {
            "left_track": "",
            "right_track": "",
            "crossfader": 0.5,
        }
        self.piano_cfg = {"keys": {}}

        self.prev_media_a = None
        self.prev_media_b = None
        self.prev_media_btn_a = 0
        self.prev_media_btn_b = 0
        self.prev_piano_key = -1
        self.prev_piano_pressed = 0

        self.pygame_available = HAS_PYGAME
        self.left_channel = None
        self.right_channel = None
        self.piano_channel = None
        self.left_sound = None
        self.right_sound = None
        self.piano_sound = None
        self.left_paused = False
        self.right_paused = False
        self.left_loaded = None
        self.right_loaded = None
        self.last_crossfader = None

        self.notes_dir = os.path.join(self.app_dir, "notes_cache")
        os.makedirs(self.notes_dir, exist_ok=True)
        self.one_shot_dir = os.path.join(self.app_dir, "one_shot_cache")
        os.makedirs(self.one_shot_dir, exist_ok=True)
        self._cleanup_one_shot_cache(max_age_days=14)

        if self.pygame_available:
            self._init_pygame_players()
        else:
            if HAS_PLAYSOUND or HAS_WINSOUND:
                self._log("[INFO] pygame not available. One-shot audio fallback is active.")
            else:
                self._log("[INFO] pygame not available. Special audio playback disabled.")

    def _log(self, msg):
        print(msg)

    def _init_pygame_players(self):
        try:
            if not pygame.mixer.get_init():
                pygame.mixer.init(frequency=44100, size=-16, channels=2, buffer=512)
            pygame.mixer.set_num_channels(8)
            self.left_channel = pygame.mixer.Channel(0)
            self.right_channel = pygame.mixer.Channel(1)
            self.piano_channel = pygame.mixer.Channel(2)
            self._log("[INFO] Using pygame audio backend.")
        except Exception as exc:
            self.pygame_available = False
            self.left_channel = None
            self.right_channel = None
            self.piano_channel = None
            self._log(f"[WARNING] pygame init failed: {exc}")

    def _cleanup_one_shot_cache(self, max_age_days=14):
        try:
            cutoff = time.time() - (max_age_days * 24 * 3600)
            for name in os.listdir(self.one_shot_dir):
                path = os.path.join(self.one_shot_dir, name)
                try:
                    if os.path.isfile(path) and os.path.getmtime(path) < cutoff:
                        os.remove(path)
                except Exception:
                    pass
        except Exception:
            pass

    def _cache_one_shot_file(self, source_path):
        src = Path(source_path)
        if not src.is_file():
            return ""
        ext = src.suffix.lower() or ".bin"
        key = f"{str(src.resolve())}|{src.stat().st_size}|{int(src.stat().st_mtime)}"
        digest = hashlib.sha1(key.encode("utf-8", errors="ignore")).hexdigest()
        target = Path(self.one_shot_dir) / f"{digest}{ext}"
        if not target.exists():
            shutil.copy2(str(src), str(target))
        return str(target)

    def update_config(self, config):
        with self.lock:
            special = config.get("special_modules", {}) if isinstance(config, dict) else {}
            if not isinstance(special, dict):
                special = {}

            media = special.get("media", {})
            if not isinstance(media, dict):
                media = {}
            self.media_cfg = {
                "left_track": str(media.get("left_track") or ""),
                "right_track": str(media.get("right_track") or ""),
                "crossfader": self._to_float(media.get("crossfader"), 0.5),
            }
            self.media_cfg["crossfader"] = min(1.0, max(0.0, self.media_cfg["crossfader"]))

            piano = special.get("piano", {})
            if not isinstance(piano, dict):
                piano = {}
            keys = piano.get("keys", {})
            if not isinstance(keys, dict):
                keys = {}

            normalized_keys = {}
            for key_id, default_note in self.DEFAULT_PIANO_NOTES.items():
                raw = keys.get(key_id, {})
                if not isinstance(raw, dict):
                    raw = {}
                normalized_keys[key_id] = {
                    "note": str(raw.get("note") or default_note).upper(),
                    "file_path": str(raw.get("file_path") or ""),
                }
            self.piano_cfg = {"keys": normalized_keys}

            self._apply_crossfader()

    def handle_media_payload(self, payload_parts):
        with self.lock:
            values = self._to_int_list(payload_parts)
            if not values:
                return

            # Expected (recommended) payload:
            # module_id=4 ; jogA ; jogB ; crossfader(0..100) ; btnA ; btnB
            jog_a = values[0] if len(values) > 0 else None
            jog_b = values[1] if len(values) > 1 else None
            cross = values[2] if len(values) > 2 else None
            btn_a = values[3] if len(values) > 3 else None
            btn_b = values[4] if len(values) > 4 else None

            if cross is not None:
                self.media_cfg["crossfader"] = min(1.0, max(0.0, float(cross) / 100.0))
                self._apply_crossfader()

            if jog_a is not None:
                self._handle_jog_delta("left", jog_a, self.prev_media_a)
                self.prev_media_a = jog_a

            if jog_b is not None:
                self._handle_jog_delta("right", jog_b, self.prev_media_b)
                self.prev_media_b = jog_b

            if btn_a is not None:
                if btn_a == 1 and self.prev_media_btn_a == 0:
                    self._toggle_deck("left")
                self.prev_media_btn_a = btn_a

            if btn_b is not None:
                if btn_b == 1 and self.prev_media_btn_b == 0:
                    self._toggle_deck("right")
                self.prev_media_btn_b = btn_b

    def handle_piano_payload(self, payload_parts):
        with self.lock:
            values = self._to_int_list(payload_parts)
            if not values:
                return

            # Expected (recommended) payload:
            # module_id=5 ; key_index(0..11 or -1) ; pressed(0/1)
            key_index = values[0]
            pressed = values[1] if len(values) > 1 else 1 if key_index >= 0 else 0

            should_play = False
            if pressed == 1:
                if self.prev_piano_pressed == 0 or key_index != self.prev_piano_key:
                    should_play = True

            self.prev_piano_key = key_index
            self.prev_piano_pressed = pressed

            if should_play and key_index >= 0:
                self._play_piano_index(key_index)

    def play_one_shot(self, file_path):
        """
        Play a generic one-shot audio file (used by standard button mappings).
        Uses the selected file directly and never falls back to synthetic beeps.
        """
        with self.lock:
            if not file_path:
                return
            path = str(file_path)
            if not os.path.isfile(path):
                if self.action_log:
                    self._log(f"[AUDIO] File not found: {path}")
                return

            cached_path = self._cache_one_shot_file(path)
            if not cached_path:
                return

            # Try pygame.music first (broader codec support, e.g. mp3/m4a).
            if self.pygame_available:
                try:
                    pygame.mixer.music.load(cached_path)
                    pygame.mixer.music.play()
                    return
                except Exception:
                    pass

                # Fallback to Sound + dedicated channel.
                if self.piano_channel is not None:
                    try:
                        self.piano_sound = pygame.mixer.Sound(cached_path)
                        self.piano_channel.play(self.piano_sound)
                        return
                    except Exception:
                        pass

            # Last resort 1: playsound (often works with OS codecs on Windows).
            if HAS_PLAYSOUND:
                try:
                    threading.Thread(
                        target=playsound,
                        args=(cached_path,),
                        kwargs={"block": False},
                        daemon=True,
                    ).start()
                    return
                except Exception:
                    pass

            # Last resort 2: native WAV fallback on Windows.
            if HAS_WINSOUND and cached_path.lower().endswith(".wav"):
                try:
                    winsound.PlaySound(cached_path, winsound.SND_FILENAME | winsound.SND_ASYNC)
                    return
                except Exception:
                    pass

            # Last resort 3: play actual file via OS player (wav on macOS/Linux fallback).
            _play_wav_fallback(cached_path)

    def close(self):
        with self.lock:
            if self.pygame_available:
                try:
                    pygame.mixer.quit()
                except Exception:
                    pass

    def _handle_jog_delta(self, deck_name, current_val, prev_val):
        if prev_val is None:
            return
        delta = current_val - prev_val
        if delta == 0:
            return

        # Convert analog delta into small seek jog.
        step_ms = int(max(-3000, min(3000, delta * 120)))
        self._jog_deck(deck_name, step_ms)

    def _toggle_deck(self, deck_name):
        if not self.pygame_available:
            return
        channel = self.left_channel if deck_name == "left" else self.right_channel
        if channel is None:
            return

        path = self.media_cfg["left_track"] if deck_name == "left" else self.media_cfg["right_track"]
        if not path or not os.path.isfile(path):
            if self.action_log:
                self._log(f"[MEDIA] No track configured for {deck_name} deck.")
            return

        loaded = self.left_loaded if deck_name == "left" else self.right_loaded
        if loaded != path:
            try:
                snd = pygame.mixer.Sound(path)
                if deck_name == "left":
                    self.left_sound = snd
                    self.left_loaded = path
                    self.left_paused = False
                else:
                    self.right_sound = snd
                    self.right_loaded = path
                    self.right_paused = False
            except Exception as exc:
                self._log(f"[WARNING] Could not load {deck_name} track (pygame): {exc}")
                return

        is_paused = self.left_paused if deck_name == "left" else self.right_paused
        if channel.get_busy() and not is_paused:
            channel.pause()
            if deck_name == "left":
                self.left_paused = True
            else:
                self.right_paused = True
            if self.event_log:
                self._log(f"[MEDIA] {deck_name} deck paused")
            return

        if is_paused:
            channel.unpause()
            if deck_name == "left":
                self.left_paused = False
            else:
                self.right_paused = False
            if self.event_log:
                self._log(f"[MEDIA] {deck_name} deck resumed")
            return

        snd = self.left_sound if deck_name == "left" else self.right_sound
        if snd is None:
            return
        channel.play(snd)
        self._apply_crossfader()
        if self.event_log:
            self._log(f"[MEDIA] {deck_name} deck playing")

    def _jog_deck(self, deck_name, delta_ms):
        # pygame mixer does not provide reliable per-track seek in this setup.
        return

    def _apply_crossfader(self):
        cross = self.media_cfg.get("crossfader", 0.5)
        left_vol = int((1.0 - cross) * 100)
        right_vol = int(cross * 100)

        if self.last_crossfader == cross:
            return

        if self.left_channel is None or self.right_channel is None:
            return
        self.left_channel.set_volume(left_vol / 100.0)
        self.right_channel.set_volume(right_vol / 100.0)

        self.last_crossfader = cross

    def _play_piano_index(self, key_index):
        if key_index < 0 or key_index >= len(self.PIANO_KEY_ORDER):
            return
        key_id = self.PIANO_KEY_ORDER[key_index]
        key_cfg = self.piano_cfg.get("keys", {}).get(key_id, {})
        note = str(key_cfg.get("note") or self.DEFAULT_PIANO_NOTES[key_id]).upper()
        custom_path = str(key_cfg.get("file_path") or "")

        file_to_play = ""
        if custom_path and os.path.isfile(custom_path):
            file_to_play = custom_path
        else:
            file_to_play = self._ensure_note_wav(note)

        self._play_audio_file(file_to_play, note)

    def _play_audio_file(self, file_path, note_for_fallback):
        if self.pygame_available and self.piano_channel is not None:
            try:
                self.piano_sound = pygame.mixer.Sound(file_path)
                self.piano_channel.play(self.piano_sound)
                return
            except Exception as exc:
                self._log(f"[WARNING] Piano playback via pygame failed: {exc}")

        if HAS_WINSOUND:
            freq = int(self.NOTE_FREQUENCIES.get(note_for_fallback, 440))
            duration_ms = 250
            try:
                winsound.Beep(freq, duration_ms)
            except Exception:
                pass
        else:
            # macOS/Linux: play generated WAV with afplay/aplay when VLC and winsound are unavailable
            wav_path = self._ensure_note_wav(note_for_fallback)
            _play_wav_fallback(wav_path)

    def _ensure_note_wav(self, note):
        frequency = self.NOTE_FREQUENCIES.get(note, self.NOTE_FREQUENCIES["C4"])
        safe_note = note.replace("#", "s")
        target = os.path.join(self.notes_dir, f"note_{safe_note}.wav")
        if os.path.isfile(target):
            return target

        sample_rate = 44100
        duration_seconds = 0.35
        sample_count = int(sample_rate * duration_seconds)

        samples = array("h")
        amplitude = 0.35
        for i in range(sample_count):
            t = float(i) / sample_rate
            value = int(math.sin(2 * math.pi * frequency * t) * amplitude * 32767)
            samples.append(value)

        with wave.open(target, "w") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(sample_rate)
            wf.writeframes(samples.tobytes())

        return target

    @staticmethod
    def _to_float(value, default):
        try:
            return float(value)
        except Exception:
            return default

    @staticmethod
    def _to_int_list(values):
        out = []
        for value in values:
            try:
                out.append(int(value))
            except Exception:
                return []
        return out
