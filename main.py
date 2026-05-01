# main.py
# J.A.R.V.I.S. entry point
#
# Run: python main.py
#
# Flow: listen (main thread) -> brain in worker thread -> speak (main thread)
# pyttsx3 requires the main thread, so we use queue.get() (blocking) to
# wait for the reply before speaking, then loop back to listen.

import threading
import queue
from dotenv import load_dotenv
load_dotenv()

from voice_module import listen, speak
from brain_module import process

DIVIDER = "-" * 50


def main() -> None:
    print("\n" + "=" * 50)
    print("   E.D.I.T.H.  --  Smart Home AI Assistant")
    print("   Press Ctrl+C to shut down")
    print("=" * 50 + "\n")
    speak("Edith online. How can I help you?")

    while True:
        # 1. Listen (main thread)
        user_text = listen()
        if not user_text:
            continue

        print(f"\n{DIVIDER}")
        print(f"  You    >> {user_text}")
        print(f"  EDITH  >> Thinking...", end="", flush=True)

        # 2. Run brain in a thread so we can show "Thinking..." while waiting
        reply_queue: queue.Queue[str] = queue.Queue()

        def _think():
            try:
                reply_queue.put(process(user_text))
            except Exception as e:
                reply_queue.put("I encountered an error. Please try again.")
                print(f"\n  ERROR  >> {e}")

        t = threading.Thread(target=_think, daemon=True)
        t.start()

        # 3. Block main thread until reply is ready (keeps pyttsx3 happy)
        reply = reply_queue.get()
        t.join()

        print(f"\r  EDITH  >> {reply}          ")
        print(DIVIDER)

        # 4. Speak on main thread — guaranteed to work
        speak(reply)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n" + "=" * 50)
        print("   E.D.I.T.H. offline. Goodbye.")
        print("=" * 50)
