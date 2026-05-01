<div align="center">

# E.D.I.T.H.
### Even Dead, I'm The Hero

**A voice-controlled AI smart home assistant powered by Google Gemini, LangChain, and ESP32**

![Python](https://img.shields.io/badge/Python-3.13-3776AB?style=for-the-badge&logo=python&logoColor=white)
![Gemini](https://img.shields.io/badge/Google_Gemini-1.5_Flash-4285F4?style=for-the-badge&logo=google&logoColor=white)
![LangChain](https://img.shields.io/badge/LangChain-1.2-1C3C3C?style=for-the-badge&logo=langchain&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-S3_WROOM-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![Firebase](https://img.shields.io/badge/Firebase-RTDB-FFCA28?style=for-the-badge&logo=firebase&logoColor=black)

</div>

---

## What is E.D.I.T.H.?

E.D.I.T.H. is a fully voice-driven AI assistant that lets you control your smart home using natural language. Speak to it, and it understands, responds, and acts.

- Say **"Turn on the light"** → it turns on the light via ESP32
- Say **"What's the temperature?"** → it reads the DHT11 sensor and tells you
- Say **"Turn off the fan and TV"** → it handles both in one command
- Say **"How are you?"** → it just chats back

Everything is spoken aloud in a **British male neural voice**.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                        main.py                          │
│              (Orchestrator / Entry Point)               │
└────────────┬────────────────────────┬───────────────────┘
             │                        │
             ▼                        ▼
┌────────────────────┐    ┌───────────────────────┐
│   voice_module.py  │    │    brain_module.py     │
│                    │    │                        │
│  STT: Google API   │    │  LangGraph ReAct Agent │
│  TTS: edge-tts     │    │  LLM: Gemini 1.5 Flash │
│  Audio: pygame     │    │  Tools: control/sensor │
└────────────────────┘    └──────────┬────────────┘
                                     │
                                     ▼
                          ┌───────────────────────┐
                          │  hardware_module.py   │
                          │                       │
                          │  HTTP → ESP32 API     │
                          │  /api/toggle?relay=N  │
                          │  /api/data            │
                          └──────────┬────────────┘
                                     │
                                     ▼
                          ┌───────────────────────┐
                          │   esp32_server.ino    │
                          │                       │
                          │  ESP32-S3-WROOM-1     │
                          │  DHT11 Sensor         │
                          │  4x Relay Module      │
                          │  Firebase RTDB Sync   │
                          └───────────────────────┘
```

---

## Features

| Feature | Details |
|---|---|
| 🎙️ Voice Input | Continuous mic listening with ambient noise calibration |
| 🔊 Voice Output | Microsoft neural British male voice (en-GB-RyanNeural) |
| 🧠 AI Brain | Google Gemini 1.5 Flash via LangGraph ReAct agent |
| 💬 Conversation Memory | Remembers last 10 exchanges for context |
| 💡 Device Control | Turn light, fan, TV, pump on/off via voice |
| 🌡️ Sensor Reading | Live temperature & humidity from DHT11 |
| ☁️ Cloud Sync | Firebase Realtime Database sync for remote control |
| 📶 Offline Mode | ESP32 falls back to local AP if Wi-Fi fails |
| 🔄 Auto Reconnect | ESP32 auto-reconnects to Wi-Fi every 30 seconds |

---

## Hardware Requirements

| Component | Details |
|---|---|
| Microcontroller | ESP32-S3-WROOM-1 |
| Temperature Sensor | DHT11 (connected to GPIO 4) |
| Relay Module | 4-channel relay (Active-LOW) |
| PC / Laptop | Running Python 3.13+ with microphone |

### Wiring

```
ESP32 GPIO 15  →  Relay 1  →  Light
ESP32 GPIO 16  →  Relay 2  →  Fan
ESP32 GPIO 17  →  Relay 3  →  TV
ESP32 GPIO 18  →  Relay 4  →  Pump
ESP32 GPIO 4   →  DHT11 Data Pin
```

> **Note:** Relays are Active-LOW — `LOW` = device ON, `HIGH` = device OFF.

---

## Project Structure

```
Project/
├── main.py               # Entry point — wires everything together
├── voice_module.py       # STT (mic → text) + TTS (text → speech)
├── brain_module.py       # LangGraph AI agent with tool-calling
├── hardware_module.py    # HTTP client for ESP32 API
├── esp32_server.ino      # Arduino firmware for ESP32
├── requirements.txt      # Python dependencies
└── .env                  # API keys (never commit this)
```

---

## Quick Start

### Step 1 — Flash the ESP32

1. Open `esp32_server.ino` in Arduino IDE
2. Install board: **ESP32 by Espressif** via Board Manager
3. Select board: `ESP32S3 Dev Module`
4. Set your credentials:
```cpp
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```
5. Upload and open Serial Monitor at **115200 baud**
6. Note the IP address printed — e.g. `192.168.1.100`

### Step 2 — Configure Python

Set the ESP32 IP in `hardware_module.py`:
```python
ESP32_IP = "192.168.1.100"   # your ESP32's IP
```

Create a `.env` file in the project root:
```
GOOGLE_API_KEY=your-gemini-api-key-here
```
> Get a free Gemini API key at [aistudio.google.com](https://aistudio.google.com/app/apikey)

### Step 3 — Install Dependencies

```bash
pip install -r requirements.txt
```

> **Windows PyAudio fix** (if pip install fails):
> ```bash
> pip install pipwin && pipwin install pyaudio
> ```

### Step 4 — Run

```bash
python main.py
```

---

## Example Conversations

```
You    >> turn on the light
EDITH  >> Done! The light is now on.

You    >> what's the temperature in the room
EDITH  >> It's currently 27.4°C with 65% humidity.

You    >> turn off the fan and the tv
EDITH  >> Done! I've turned off the fan and the TV.

You    >> which devices are on right now
EDITH  >> Currently, the light is on. The fan, TV, and pump are all off.

You    >> thank you goodbye
EDITH  >> Goodbye! I'll be here if you need me.
```

---

## Configuration Reference

### Change the AI Voice

Edit `voice_module.py`:

```python
VOICE = "en-GB-RyanNeural"          # British male (default)
# VOICE = "en-US-ChristopherNeural" # Deep American male
# VOICE = "en-US-GuyNeural"         # Neutral American male
```

### Change the AI Model

Edit `brain_module.py`:

```python
model="gemini-1.5-flash"    # fast, free-tier friendly (default)
# model="gemini-1.5-pro"    # more powerful reasoning
```

### Add a New Device

1. Wire a new relay to an ESP32 GPIO pin
2. Add to `DEVICE_RELAY_MAP` in `hardware_module.py`:
```python
DEVICE_RELAY_MAP = {
    "light": 1,
    "fan":   2,
    "tv":    3,
    "pump":  4,
    "ac":    5,   # new device
}
```
3. Add the GPIO pin to `esp32_server.ino` and handle relay 5 in the toggle route

---

## Dependencies

| Package | Purpose |
|---|---|
| `SpeechRecognition` | Microphone → text (STT) |
| `edge-tts` | Microsoft neural voice synthesis |
| `pygame` | Audio playback |
| `pyaudio` | Microphone access |
| `langchain` | Agent framework |
| `langchain-google-genai` | Gemini LLM integration |
| `langgraph` | ReAct agent execution |
| `google-generativeai` | Google AI SDK |
| `requests` | HTTP calls to ESP32 |
| `python-dotenv` | Load `.env` API keys |

---

## Security Notes

- **Never commit `.env`** — add it to `.gitignore`
- **Never hardcode API keys** in source files
- The ESP32 has no authentication — keep it on a trusted local network

---

## Troubleshooting

| Problem | Fix |
|---|---|
| No audio output | Check internet connection (edge-tts needs it) |
| `KeyError: GOOGLE_API_KEY` | Make sure `.env` exists with your key |
| ESP32 not reachable | Verify `ESP32_IP` matches Serial Monitor output |
| Mic not detected | Install PyAudio correctly; check mic permissions |
| `ModuleNotFoundError` | Run `pip install -r requirements.txt` |

---

<div align="center">

Built with Python, LangChain, Google Gemini, and ESP32

</div>
