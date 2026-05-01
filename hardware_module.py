# hardware_module.py
# Communicates with the ESP32 Smart Home server.
#
# Your ESP32 API (from esp32_server.ino):
#   GET /api/data              -> JSON with relay states, temp, humidity
#   GET /api/toggle?relay=<N>  -> toggles relay N (1-4), returns {"status":"success"}
#
# Relay mapping (active-LOW: LOW=ON, HIGH=OFF):
#   Relay 1 = Light  (GPIO 15)
#   Relay 2 = Fan    (GPIO 16)
#   Relay 3 = TV     (GPIO 17)
#   Relay 4 = Pump   (GPIO 18)
#
# Install: pip install requests

import requests

# --- Configuration ---
ESP32_IP = "192.168.1.6"  # <-- Replace with IP from ESP32 Serial Monitor
ESP32_PORT = 80
REQUEST_TIMEOUT = 5
BASE_URL = f"http://{ESP32_IP}:{ESP32_PORT}"

# Device name -> relay number
DEVICE_RELAY_MAP: dict[str, int] = {
    "light": 1,
    "fan":   2,
    "tv":    3,
    "pump":  4,
}


def _get_state() -> dict | None:
    """Fetch current relay states and sensor data from ESP32."""
    try:
        r = requests.get(f"{BASE_URL}/api/data", timeout=REQUEST_TIMEOUT)
        r.raise_for_status()
        return r.json()
    except Exception:
        return None


def control_hardware(device_name: str, state: str) -> str:
    """
    Turn a smart home device on or off.

    The ESP32 /api/toggle endpoint only toggles, so we first read the current
    state via /api/data and only send the toggle request if the device is not
    already in the desired state.

    Args:
        device_name: 'light', 'fan', 'tv', or 'pump'
        state:       'on' or 'off'

    Returns:
        Human-readable result string for the AI agent.
    """
    device_name = device_name.lower().strip()
    state = state.lower().strip()

    relay_num = DEVICE_RELAY_MAP.get(device_name)
    if relay_num is None:
        return f"Unknown device '{device_name}'. Available: {list(DEVICE_RELAY_MAP.keys())}"

    if state not in ("on", "off"):
        return f"Invalid state '{state}'. Use 'on' or 'off'."

    # Read current state to avoid unnecessary toggles
    current_data = _get_state()
    if current_data is None:
        return f"Could not reach ESP32 at {ESP32_IP}. Check Wi-Fi and IP address."

    relay_key = f"relay{relay_num}"
    is_on: bool = current_data.get(relay_key, False)
    desired_on = (state == "on")

    if is_on == desired_on:
        return f"{device_name.capitalize()} is already {state}. No change needed."

    # Send toggle request
    try:
        r = requests.get(
            f"{BASE_URL}/api/toggle",
            params={"relay": relay_num},
            timeout=REQUEST_TIMEOUT,
        )
        r.raise_for_status()
        return f"{device_name.capitalize()} turned {state} successfully."
    except requests.exceptions.ConnectionError:
        return f"Could not connect to ESP32 at {ESP32_IP}."
    except requests.exceptions.Timeout:
        return f"ESP32 request timed out after {REQUEST_TIMEOUT}s."
    except requests.exceptions.HTTPError as e:
        return f"ESP32 returned an error: {e}"


def get_sensor_data() -> str:
    """
    Read temperature, humidity, and all relay states from the ESP32.
    Returns a human-readable summary string for the AI agent.
    """
    data = _get_state()
    if data is None:
        return f"Could not reach ESP32 at {ESP32_IP}."

    relay_status = ", ".join(
        f"{name} {'ON' if data.get(f'relay{num}', False) else 'OFF'}"
        for name, num in DEVICE_RELAY_MAP.items()
    )
    return (
        f"Temperature: {data.get('temperature', 'N/A')}°C, "
        f"Humidity: {data.get('humidity', 'N/A')}%, "
        f"Devices — {relay_status}, "
        f"Mode: {data.get('mode', 'Unknown')}"
    )
