import serial
import time
import json
import webbrowser
import subprocess
import pyautogui
import screen_brightness_control as sbc
import os
import threading
import platform
from pathlib import Path
import queue
import psutil
from fastapi import FastAPI
from threading import Thread, Event
import uvicorn
from serial.tools import list_ports
from module_extensions import SpecialModuleManager

# Audio Utils (pycaw – Windows only; on macOS/Linux we use key simulation)
volume = None
HAS_PYCAW = False
if platform.system() == "Windows":
    try:
        from ctypes import cast, POINTER
        from comtypes import CLSCTX_ALL
        from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
        devices = AudioUtilities.GetSpeakers()
        interface = devices.Activate(
            IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        volume = cast(interface, POINTER(IAudioEndpointVolume))
        HAS_PYCAW = True
        print("[INFO] pycaw initialized successfully. Using direct volume control.")
    except Exception as e:
        print(f"[WARNING] pycaw initialization failed: {e}")
        print("[WARNING] Falling back to simulating key presses for volume control.")

# GPU Utils (Optional)
try:
    import GPUtil
    HAS_GPUTIL = True
except ImportError:
    HAS_GPUTIL = False

# Debug / Logging Configuration
# Printing telemetry every 0.5s can cause micro-stutters on some systems due to console I/O.
ENABLE_TELEMETRY_LOG = os.getenv("CONSOLE_DECK_PRO_TELEMETRY_LOG", "0") in ("1", "true", "True")
ENABLE_EVENT_LOG = os.getenv("CONSOLE_DECK_PRO_EVENT_LOG", "0") in ("1", "true", "True")
ENABLE_ACTION_LOG = os.getenv("CONSOLE_DECK_PRO_ACTION_LOG", "0") in ("1", "true", "True")
ANALOG_ABSOLUTE_DEADBAND = 2

APP_NAME = "ConsoleDeckPro"

def _default_app_dir() -> str:
    """
    Cross-platform, user-writable config directory.
    Override with CONSOLE_DECK_PRO_DIR or CONSOLE_DECK_PRO_CONFIG.
    """
    override = os.getenv("CONSOLE_DECK_PRO_DIR")
    if override:
        return override

    system = platform.system()
    home = Path.home()

    if system == "Windows":
        appdata = os.getenv("APPDATA") or os.getenv("LOCALAPPDATA")
        if appdata:
            return str(Path(appdata) / APP_NAME)
        return str(home / "AppData" / "Roaming" / APP_NAME)

    if system == "Darwin":
        return str(home / "Library" / "Application Support" / APP_NAME)

    xdg = os.getenv("XDG_CONFIG_HOME")
    if xdg:
        return str(Path(xdg) / APP_NAME)
    return str(home / ".config" / APP_NAME)

APP_DIR = _default_app_dir()
CONFIG_FILE = os.getenv("CONSOLE_DECK_PRO_CONFIG") or os.path.join(APP_DIR, "config.json")

# Ensure the app directory exists
os.makedirs(APP_DIR, exist_ok=True)

app = FastAPI()
config_data = None
config_updated_event = Event() # Used to signal the main thread to reload the config
last_volume_target = None
# For slider brightness: store previous raw value so we apply delta, not absolute
_last_brightness_slider_raw = None
special_module_manager = SpecialModuleManager(
    APP_DIR,
    event_log=ENABLE_EVENT_LOG,
    action_log=ENABLE_ACTION_LOG,
)

@app.post("/reload")
def reload_config_endpoint():
    """
    Called by the UI when the config is saved.
    This simply sets an event to notify the main loop to re-read the config file.
    """
    config_updated_event.set()
    print("Configuration reload requested via API")
    return {"message": "Config reload signaled"}

@app.get("/serial/ports")
def get_serial_ports():
    ports = list_ports.comports()
    return [{"device": port.device, "description": port.description} for port in ports]


@app.get("/status")
def get_backend_status():
    cfg = config_data if isinstance(config_data, dict) else load_config()
    cfg = cfg if isinstance(cfg, dict) else {}
    serial_cfg = cfg.get("serial", {})
    port = serial_cfg.get("port") if isinstance(serial_cfg, dict) else None
    return {"running": True, "configured": bool(port), "port": port}


def run_server():
    # Uvicorn logging can be noisy, this can be changed to 'info' for more detail
    uvicorn.run(app, host="127.0.0.1", port=8000, log_level="warning")

def load_config():
    if not os.path.exists(CONFIG_FILE):
        print(f"Info: {CONFIG_FILE} not found. Creating a new one with default values.")
        default_config = {
            "serial": {
                "port": None,
                "baud_rate": 115200
            },
            "mappings": {}
        }
        try:
            with open(CONFIG_FILE, 'w') as f:
                json.dump(default_config, f, indent=4)
            return default_config
        except Exception as e:
            print(f"Error: Could not create default config file: {e}")
            return None

    try:
        with open(CONFIG_FILE, 'r') as f:
            content = f.read()
            if not content.strip():
                print(f"Warning: {CONFIG_FILE} is empty. Using default values.")
                return {"serial": {"port": None}, "mappings": {}}
            return json.loads(content)
    except json.JSONDecodeError as e:
        print(f"Error parsing {CONFIG_FILE}: {e}. Backing up and resetting to defaults.")
        try:
            backup_path = f"{CONFIG_FILE}.bad.{int(time.time())}"
            os.replace(CONFIG_FILE, backup_path)
            print(f"Info: bad config moved to {backup_path}")
        except Exception:
            pass
        try:
            with open(CONFIG_FILE, 'w') as wf:
                json.dump(default_config, wf, indent=4)
            return default_config
        except Exception:
            return None
    except Exception as e:
        print(f"Error reading {CONFIG_FILE}: {e}")
        return None

def ensure_config_loaded():
    global config_data
    if not isinstance(config_data, dict):
        config_data = load_config()
    if not isinstance(config_data, dict):
        config_data = {"serial": {"port": None, "baud_rate": 115200}, "mappings": {}, "special_modules": {}}
    return config_data


def execute_action(action_def, absolute_value=None, multiplier=1):
    global last_volume_target, _last_brightness_slider_raw
    if not action_def or not isinstance(action_def, dict):
        return

    action_type = action_def.get('action')
    value = action_def.get('value')

    if not action_type:
        return

    if absolute_value is not None:
        value = absolute_value

    if ENABLE_ACTION_LOG:
        print(f"  -> [ACTION] {action_type}: {value}")

    try:
        if action_type == 'open_url':
            if ENABLE_ACTION_LOG:
                print(f"     Opening URL: {value}")
            webbrowser.open(value)
        elif action_type == 'open_app':
            if ENABLE_ACTION_LOG:
                print(f"     Opening App: {value}")
            subprocess.Popen(value)
        elif action_type == 'hotkey':
            if ENABLE_ACTION_LOG:
                print(f"     Sending Hotkey: {value}")
            if isinstance(value, list):
                pyautogui.hotkey(*value)
            else:
                pyautogui.press(value)
        elif action_type == 'type':
            if ENABLE_ACTION_LOG:
                print(f"     Typing: {value}")
            pyautogui.write(value)
        elif action_type == 'brightness':
            # Relative: current + value (encoder/button)
            if ENABLE_ACTION_LOG:
                print(f"     Changing Brightness (delta): {value}")
            try:
                current = sbc.get_brightness()
                if isinstance(current, list): current = current[0]
                new_val = min(100, max(0, current + int(value)))
                sbc.set_brightness(new_val)
                if ENABLE_ACTION_LOG:
                    print(f"     Brightness: {current} -> {new_val}")
            except Exception as e:
                print(f"     [ERROR] Brightness control failed: {e}")
        elif action_type == 'set_volume':
            target = max(0, min(100, int(float(value))))
            if last_volume_target is None:
                last_volume_target = target
                return
            delta = target - last_volume_target
            if abs(delta) < ANALOG_ABSOLUTE_DEADBAND:
                return
            key = 'volumeup' if delta > 0 else 'volumedown'
            count = min(30, max(1, int(round(abs(delta) / 2.0))))
            if ENABLE_ACTION_LOG:
                print(f"     Set Volume (approx): {key} x {count} (target {target}%)")
            for _ in range(count):
                pyautogui.press(key)
            last_volume_target = target
        elif action_type == 'toggle_mute':
            pyautogui.press('volumemute')
            if ENABLE_ACTION_LOG:
                print("     Toggled Mute")
        elif action_type == 'change_volume':
            key = 'volumeup' if float(value) > 0 else 'volumedown'
            count = int(round(abs(float(value)) * multiplier))
            if count < 1: count = 1
            if count > 25: count = 25
            if ENABLE_ACTION_LOG:
                print(f"     Change Volume: {key} x {count}")
            for _ in range(count):
                pyautogui.press(key)
        elif action_type == 'set_brightness':
            # Relative everywhere: from slider use delta of position; from config use value as delta
            try:
                current = sbc.get_brightness()
                if isinstance(current, list): current = current[0]
                if absolute_value is not None:
                    # Slider: apply difference from previous position (raw 0–1023)
                    raw = int(float(absolute_value))
                    if _last_brightness_slider_raw is None:
                        _last_brightness_slider_raw = raw
                        return
                    delta_raw = raw - _last_brightness_slider_raw
                    _last_brightness_slider_raw = raw
                    # Scale slider delta to brightness delta (~0–1023 -> ~0–100)
                    delta_pct = round(delta_raw * 100.0 / 1023.0)
                    new_val = min(100, max(0, current + delta_pct))
                else:
                    # Button/config: value is delta
                    delta_pct = int(value)
                    new_val = min(100, max(0, current + delta_pct))
                sbc.set_brightness(new_val)
                if ENABLE_ACTION_LOG:
                    print(f"     Brightness: {current} -> {new_val} (relative)")
            except Exception as e:
                print(f"     [ERROR] Set Brightness failed: {e}")
        else:
            print(f"     [WARNING] Unknown action type: {action_type}")
    except Exception as e:
        print(f"     [ERROR] Failed to execute action: {e}")


def reload_config_if_requested():
    global config_data
    if config_updated_event.is_set():
        config_data = load_config()
        config_updated_event.clear()
        sync_special_module_manager()


def sync_special_module_manager():
    global config_data
    ensure_config_loaded()
    if isinstance(config_data, dict):
        special_module_manager.update_config(config_data)

def _get_gpu_stats():
    """
    Best-effort GPU utilization (%) and temperature (C).
    Returns (-1, -1) when unavailable.
    """
    if HAS_GPUTIL:
        try:
            gpus = GPUtil.getGPUs()
            if gpus:
                return int(gpus[0].load * 100), int(gpus[0].temperature)
        except Exception:
            pass

    try:
        # Avoid shell=True for performance and safety.
        proc = subprocess.run(
            [
                "nvidia-smi",
                "--query-gpu=utilization.gpu,temperature.gpu",
                "--format=csv,noheader,nounits",
            ],
            capture_output=True,
            text=True,
            timeout=1.0,
            check=False,
        )
        out = (proc.stdout or "").strip()
        if out:
            parts = [p.strip() for p in out.split(",")]
            if len(parts) >= 2:
                return int(parts[0]), int(parts[1])
    except Exception:
        pass

    return -1, -1


class ActionExecutor:
    """
    Executes actions on a background thread so serial I/O stays responsive.
    """

    def __init__(self, max_queue=256):
        self._q = queue.Queue(maxsize=max_queue)
        self._stop = Event()
        self._thread = Thread(target=self._run, daemon=True)

    def start(self):
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()

    def submit(self, action_def, absolute_value=None, multiplier=1):
        if not action_def:
            return
        item = (action_def, absolute_value, multiplier)
        try:
            self._q.put_nowait(item)
        except queue.Full:
            # Drop one item to keep latency bounded.
            try:
                self._q.get_nowait()
                self._q.task_done()
            except queue.Empty:
                pass
            try:
                self._q.put_nowait(item)
            except queue.Full:
                pass

    def _run(self):
        while not self._stop.is_set():
            try:
                action_def, absolute_value, multiplier = self._q.get(timeout=0.2)
            except queue.Empty:
                continue
            try:
                execute_action(action_def, absolute_value=absolute_value, multiplier=multiplier)
            except Exception as e:
                print(f"     [ERROR] Action worker failed: {e}")
            finally:
                self._q.task_done()


class TelemetrySampler:
    """
    Samples telemetry on a background thread to avoid blocking the serial loop.
    """

    def __init__(self, interval_s=0.5, slow_interval_s=2.0):
        self._interval_s = float(interval_s)
        self._slow_interval_s = float(slow_interval_s)
        self._stop = Event()
        self._lock = threading.Lock()
        self._thread = Thread(target=self._run, daemon=True)
        self._snapshot = {
            "cpu": 0,
            "gpu": -1,
            "ram": 0,
            "temp_cpu": -1,
            "temp_gpu": -1,
        }

    def start(self):
        # Prime cpu_percent() so the first read isn't 0/meaningless.
        try:
            psutil.cpu_percent(interval=None)
        except Exception:
            pass
        self._thread.start()
        return self

    def stop(self):
        self._stop.set()

    def get_snapshot(self):
        with self._lock:
            return dict(self._snapshot)

    def _run(self):
        last_time = time.monotonic()
        next_slow = 0.0
        gpu = -1
        temp_gpu = -1
        temp_cpu = -1

        while not self._stop.is_set():
            now = time.monotonic()

            # Fast metrics (every tick)
            try:
                cpu = int(psutil.cpu_percent(interval=None))
            except Exception:
                cpu = 0

            try:
                ram = int(psutil.virtual_memory().percent)
            except Exception:
                ram = 0

            # Slow metrics (temperature/GPU) on a slower cadence
            if now >= next_slow:
                gpu, temp_gpu = _get_gpu_stats()
                temp_cpu = get_cpu_temp()
                if temp_cpu <= 0:
                    temp_cpu = -1
                next_slow = now + self._slow_interval_s

            last_time = now

            with self._lock:
                self._snapshot.update(
                    {
                        "cpu": cpu,
                        "gpu": gpu,
                        "ram": ram,
                        "temp_cpu": temp_cpu,
                        "temp_gpu": temp_gpu,
                    }
                )

            self._stop.wait(self._interval_s)

def main():
    global config_data
    server_thread = Thread(target=run_server)
    server_thread.daemon = True
    server_thread.start()
    
    # Load the initial configuration
    config_data = load_config()
    if not config_data:
        print("Error: Could not load or create a configuration file. Exiting.")
        return
    sync_special_module_manager()

    # --- Main Loop ---
    while True:
        try:
            reload_config_if_requested()
            ensure_config_loaded()

            serial_config = config_data.get('serial', {})
            port = serial_config.get('port')
            baud_rate = serial_config.get('baud_rate', 9600)

            # STATE 1: Port is not configured. Wait for the signal or a timeout.
            if not port:
                print("\r[INFO] Serial port not configured. Waiting for selection in the UI...", end="", flush=True)
                # Wait for the reload signal, with a 2-second timeout to keep the loop spinning
                config_updated_event.wait(timeout=2)
                continue

            # STATE 2: Port is configured. Attempt connection and run device logic.
            print(f"\n[INFO] Port configured. Attempting to connect to {port} at {baud_rate}...")
            try:
                ser = serial.Serial(port, baud_rate, timeout=1)
                time.sleep(2) # Wait for connection to settle
                print("[SUCCESS] Connected to device!")
            except serial.SerialException as e:
                print(f"[ERROR] Failed to connect to serial port '{port}': {e}")
                if "PermissionError" in str(e) or "Access is denied" in str(e):
                    print("[TIP] The port might be in use by another application.")
                print("[INFO] Will retry in 10 seconds. You can change the port in the UI.")
                time.sleep(10)
                config_data = load_config() # Reload config before retrying
                continue

            # --- This is the main device loop ---
            run_device_loop(ser)
            
            # If the loop exits (e.g., device disconnect), inform the user and wait before retrying.
            print("\n[INFO] Device disconnected. Waiting for reconnection or configuration change...")
            time.sleep(5)
            
        except KeyboardInterrupt:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"\n[FATAL] An unexpected error occurred in the main loop: {e}")
            print("[INFO] Restarting main loop in 10 seconds...")
            time.sleep(10)


def run_device_loop(ser):
    """
    Handles all the communication with the serial device.
    This function is extracted from main() to be called once a connection is established.
    """
    global config_data
    
    prev_main_btns = [0] * 9
    # These need to be lists to be mutable and have their state changed by the child function.
    prev_enc_click = [0]
    prev_enc_val = [None]
    prev_ext_btns = [0] * 6
    prev_slider_vals = [None, None]
    prev_knob_vals = [None, None]

    action_executor = ActionExecutor().start()
    telemetry = TelemetrySampler(interval_s=0.5, slow_interval_s=2.0).start()

    stats_interval_s = 0.5
    next_stats_time = time.monotonic()

    try:
        while True:  # This loop will run as long as the device is connected
            try:
                if config_updated_event.is_set():
                    print("\n[INFO] Reloading configuration in device loop...")
                    config_data = load_config()
                    config_updated_event.clear()

                now = time.monotonic()
                # Short timeout so we never block long: key presses are read within ~20ms
                timeout = max(0.0, min(0.02, next_stats_time - now))
                ser.timeout = timeout

                raw = ser.readline()
                if raw:
                    text = raw.decode("utf-8", errors="ignore")
                    for line in text.splitlines():
                        line = line.strip()
                        if not line:
                            continue
                        process_serial_line(
                            line,
                            prev_main_btns,
                            prev_enc_click,
                            prev_enc_val,
                            prev_ext_btns,
                            prev_slider_vals,
                            prev_knob_vals,
                            action_executor,
                        )
                    # Drain remaining lines so we don't lag behind Arduino
                    while ser.in_waiting:
                        raw = ser.readline()
                        if not raw:
                            break
                        text = raw.decode("utf-8", errors="ignore")
                        for line in text.splitlines():
                            line = line.strip()
                            if not line:
                                continue
                            process_serial_line(
                                line,
                                prev_main_btns,
                                prev_enc_click,
                                prev_enc_val,
                                prev_ext_btns,
                                prev_slider_vals,
                                prev_knob_vals,
                                action_executor,
                            )

                now = time.monotonic()
                if now >= next_stats_time:
                    snap = telemetry.get_snapshot()
                    cpu = snap["cpu"]
                    gpu = snap["gpu"]
                    ram = snap["ram"]
                    temp_cpu = snap["temp_cpu"]
                    temp_gpu = snap["temp_gpu"]

                    stats_msg = f"STATS:{cpu},{gpu},{ram},{temp_cpu},{temp_gpu}\n"
                    ser.write(stats_msg.encode("utf-8"))
                    if ENABLE_TELEMETRY_LOG:
                        gpu_str = f"{gpu}%" if gpu != -1 else "N/A"
                        tq_str = f"{temp_cpu}C" if temp_cpu != -1 else "N/A"
                        tg_str = f"{temp_gpu}C" if temp_gpu != -1 else "N/A"
                        print(
                            f"\r[STATS] CPU:{cpu}% RAM:{ram}% GPU:{gpu_str} | C:{tq_str} G:{tg_str}",
                            end="",
                        )

                    # Keep cadence stable even if we fell behind.
                    next_stats_time = now + stats_interval_s

            except (serial.SerialException, OSError) as e:
                print(f"\n[ERROR] Device disconnected or serial error: {e}")
                try:
                    ser.close()
                except Exception:
                    pass
                return
            except Exception as e:
                print(f"\n[ERROR] An error occurred in device loop: {e}")
                time.sleep(0.2)
    finally:
        telemetry.stop()
        action_executor.stop()


def process_serial_line(line, prev_main_btns, prev_enc_click, prev_enc_val, prev_ext_btns, prev_slider_vals, prev_knob_vals, action_executor):
    """
    Parses a single line of serial data and executes actions.
    This function is extracted to be callable for each line received.
    """
    global config_data

    try:
        mappings = config_data.get('mappings', {}) if isinstance(config_data, dict) else {}
        parts = line.split(';')
        if len(parts) < 12: 
            return

        current_main_btns = [int(x) for x in parts[0:9]]
        current_enc_click = int(parts[9])
        current_enc_val = int(parts[10])
        module_id = int(parts[11])

        for i in range(9):
            if current_main_btns[i] == 1 and prev_main_btns[i] == 0:
                if ENABLE_EVENT_LOG:
                    print(f"[EVENT] Main Button {i+1} Pressed")
                btn_key = f"btn_{i+1}"
                action_executor.submit(mappings.get(btn_key))
        # Update state for the next line processing
        for i in range(9):
            prev_main_btns[i] = current_main_btns[i]

        # Encoder Button Logic (Short vs Long Press) is stateful and complex,
        # for now, we assume the user doesn't press and rotate between serial messages.
        # for now, we assume the user doesn't press and rotate between serial messages.
        # This part might need more robust state management if issues arise.
        if current_enc_click == 1 and prev_enc_click[0] == 0:
            if ENABLE_EVENT_LOG:
                print(f"[EVENT] Encoder Clicked (Short)")
            execute_action(mappings.get('enc_click'))
        prev_enc_click[0] = current_enc_click

        if prev_enc_val[0] is not None:
            diff = current_enc_val - prev_enc_val[0]
            if diff != 0:
                multiplier = abs(diff) / 2.0
                if diff > 0:
                    if ENABLE_EVENT_LOG:
                        print(f"[EVENT] Encoder Rotated CW (x{multiplier})")
                    execute_action(mappings.get('enc_cw'), multiplier=multiplier)
                elif diff < 0:
                    if ENABLE_EVENT_LOG:
                        print(f"[EVENT] Encoder Rotated CCW (x{multiplier})")
                    execute_action(mappings.get('enc_ccw'), multiplier=multiplier)
        prev_enc_val[0] = current_enc_val

        if module_id == 1: # Buttons
            if len(parts) >= 18:
                current_ext_btns = [int(x) for x in parts[12:18]]
                for i in range(6):
                        if current_ext_btns[i] == 1 and prev_ext_btns[i] == 0:
                            if ENABLE_EVENT_LOG:
                                print(f"[EVENT] Ext Button {i+1} Pressed")
                            btn_key = f"ext_btn_{i+1}"
                            action_executor.submit(mappings.get(btn_key))
                for i in range(6):
                    prev_ext_btns[i] = current_ext_btns[i]
                
        elif module_id == 2 or module_id == 3: # Sliders or Knobs
            if len(parts) >= 14:
                val1 = int(parts[12])
                val2 = int(parts[13])
                
                is_slider = (module_id == 2)
                prefix = "slider" if is_slider else "knob"
                prev_vals = prev_slider_vals if is_slider else prev_knob_vals
                
                def handle_analog(curr, prev, name):
                    abs_mapping = mappings.get(name)
                    if abs_mapping:
                        if curr != prev:
                            action_executor.submit(abs_mapping, absolute_value=curr)
                        return curr

                    if prev is None:
                        return curr
                    diff = curr - prev
                    if abs(diff) > 2:
                        if diff > 0:
                            print(f"[EVENT] {name} Moved UP/CW (Val: {curr})")
                            action_executor.submit(mappings.get(f"{name}_up" if is_slider else f"{name}_cw"))
                        else:
                            print(f"[EVENT] {name} Moved DOWN/CCW (Val: {curr})")
                            action_executor.submit(mappings.get(f"{name}_down" if is_slider else f"{name}_ccw"))
                        return curr
                    return prev

                prev_vals[0] = handle_analog(val1, prev_vals[0], f"{prefix}_1")
                prev_vals[1] = handle_analog(val2, prev_vals[1], f"{prefix}_2")
        elif module_id == 4:
            special_module_manager.handle_media_payload(parts[12:])
        elif module_id == 5:
            special_module_manager.handle_piano_payload(parts[12:])
    
    except (ValueError, IndexError) as e:
        print(f"\n[WARNING] Could not parse serial line: '{line}'. Error: {e}")

# --- HELPER FUNCTIONS ---
def get_gpu_stats():
    gpu = -1
    temp_gpu = -1

    if HAS_GPUTIL:
        try:
            gpus = GPUtil.getGPUs()
            if gpus:
                return int(gpus[0].load * 100), int(gpus[0].temperature)
        except Exception:
            pass

    try:
        output = subprocess.check_output(
            [
                "nvidia-smi",
                "--query-gpu=utilization.gpu,temperature.gpu",
                "--format=csv,noheader,nounits",
            ],
            stderr=subprocess.DEVNULL,
            timeout=1,
        ).decode("utf-8").strip()
        parts_gpu = output.split(',')
        if len(parts_gpu) >= 2:
            gpu = int(parts_gpu[0].strip())
            temp_gpu = int(parts_gpu[1].strip())
    except Exception:
        pass

    return gpu, temp_gpu


def get_cpu_temp():
    # Try psutil
    t = -1
    try:
        temps = psutil.sensors_temperatures()
        if temps:
            for name, entries in temps.items():
                if 'cpu' in name.lower() or 'core' in name.lower() or 'package' in name.lower():
                    val = int(entries[0].current)
                    if val > 0: 
                        t = val
                        break
    except Exception:
        pass
    
    # If invalid, Try WMI (Windows only)
    if t <= 0 and platform.system() == "Windows":
        try:
            ps_cmd = "Get-WmiObject MSAcpi_ThermalZoneTemperature -Namespace \"root/wmi\" | Select -ExpandProperty CurrentTemperature"
            kwargs = {}
            # CREATE_NO_WINDOW is not defined on non-Windows and may not exist in some environments.
            if hasattr(subprocess, "CREATE_NO_WINDOW"):
                kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
            out = subprocess.check_output(["powershell", "-c", ps_cmd], **kwargs).decode().strip()
            if out and out.isdigit():
                kelvin_x10 = int(out)
                cels = (kelvin_x10 / 10.0) - 273.15
                t = int(cels)
        except Exception:
            pass
    return t

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[INFO] Program terminated by user.")
    except Exception as e:
        print(f"\n[FATAL] An unhandled exception occurred in main: {e}")
    finally:
        try:
            special_module_manager.close()
        except Exception:
            pass
        
    try:
        if os.isatty(0):
            input("\n[INFO] Press Enter to close this window...")
    except Exception:
        pass
