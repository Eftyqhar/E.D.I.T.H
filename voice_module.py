# voice_module.py
# STT: SpeechRecognition + Google
# TTS: edge-tts (Microsoft neural male voice) + pygame for playback
#
# Install: pip install SpeechRecognition pyaudio edge-tts pygame
#
# Available male voices (swap VOICE below):
#   en-US-GuyNeural         -- neutral American male
#   en-US-ChristopherNeural -- deeper American male
#   en-GB-RyanNeural        -- British male (closest to EDITH)

import os
import asyncio
import warnings
import tempfile

os.environ["PYGAME_HIDE_SUPPORT_PROMPT"] = "1"
warnings.filterwarnings("ignore")

import pygame
import edge_tts
import speech_recognition as sr

# --- TTS config ---
VOICE = "en-GB-RyanNeural"   # British male — closest to the real JARVIS
RATE  = "+10%"               # slightly faster than default

pygame.mixer.init()


async def _synthesize(text: str, path: str) -> None:
    communicate = edge_tts.Communicate(text, VOICE, rate=RATE)
    await communicate.save(path)


def speak(text: str) -> None:
    """Convert text to speech using Microsoft neural voice and play via pygame."""
    try:
        with tempfile.NamedTemporaryFile(delete=False, suffix=".mp3") as f:
            tmp_path = f.name

        asyncio.run(_synthesize(text, tmp_path))

        pygame.mixer.music.load(tmp_path)
        pygame.mixer.music.play()
        while pygame.mixer.music.get_busy():
            pygame.time.wait(50)

        pygame.mixer.music.unload()
        os.remove(tmp_path)
    except Exception as e:
        print(f"  TTS error >> {e}")


# --- STT setup ---
_recognizer = sr.Recognizer()
_recognizer.pause_threshold = 1.0
_recognizer.dynamic_energy_threshold = True


def listen() -> str | None:
    """Listen from mic and return transcribed text, or None on failure."""
    with sr.Microphone() as source:
        print("  * Listening...", end="", flush=True)
        _recognizer.adjust_for_ambient_noise(source, duration=0.5)
        try:
            audio = _recognizer.listen(source, timeout=8, phrase_time_limit=15)
        except sr.WaitTimeoutError:
            print("\r  * No speech detected.          ")
            return None

    try:
        text = _recognizer.recognize_google(audio)
        print(f"\r  * Heard: {text}                 ")
        return text
    except sr.UnknownValueError:
        print("\r  * Could not understand.         ")
        return None
    except sr.RequestError as e:
        print(f"\r  * STT error: {e}               ")
        return None
