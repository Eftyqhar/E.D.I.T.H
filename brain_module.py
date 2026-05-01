# brain_module.py
# LangChain/LangGraph agent with Gemini and hardware tools.
#
# Compatible with: langchain>=1.0, langchain-google-genai>=1.0, langgraph>=1.0
#
# Set your Gemini API key:
#   Windows:  set GOOGLE_API_KEY=AIza...
#   Linux/Mac: export GOOGLE_API_KEY=AIza...

import os
from dotenv import load_dotenv
load_dotenv()  # loads GOOGLE_API_KEY from .env

from langchain_google_genai import ChatGoogleGenerativeAI
from langchain_core.tools import tool
from langchain_core.messages import HumanMessage, SystemMessage
from langgraph.prebuilt import create_react_agent

from hardware_module import (
    control_hardware as _hw_control,
    get_sensor_data as _hw_sensors,
)

# --- Tools ---

@tool
def control_hardware(device_name: str, state: str) -> str:
    """
    Turn a smart home device on or off.
    Use this when the user asks to turn on/off a light, fan, TV, or pump.

    Args:
        device_name: 'light', 'fan', 'tv', or 'pump'
        state: 'on' or 'off'
    """
    return _hw_control(device_name, state)


@tool
def get_sensor_data() -> str:
    """
    Get the current temperature, humidity, and on/off status of all devices
    from the ESP32. Use this when the user asks about room conditions,
    sensor readings, or which devices are currently on.
    """
    return _hw_sensors()


# --- LLM ---
_llm = ChatGoogleGenerativeAI(
    model="gemini-2.5-flash-lite",
    temperature=0.4,
    google_api_key=os.environ["GOOGLE_API_KEY"],
)

# --- System prompt ---
_SYSTEM = (
    "You are E.D.I.T.H., a witty, concise, and highly capable AI assistant. "
    "You control a smart home with 4 devices: light, fan, TV, and pump. "
    "You can also read room temperature and humidity from sensors. "
    "Keep responses short and conversational — this is a voice interface. "
    "After controlling hardware or reading sensors, confirm the result naturally."
)

# --- Agent (LangGraph ReAct) ---
_agent = create_react_agent(
    model=_llm,
    tools=[control_hardware, get_sensor_data],
    prompt=_SYSTEM,
)

# Rolling conversation history (list of HumanMessage / AIMessage)
_chat_history: list = []
_MAX_HISTORY = 10


def process(user_input: str) -> str:
    """Send user text to the agent and return the assistant's reply."""
    global _chat_history

    messages = _chat_history + [HumanMessage(content=user_input)]

    result = _agent.invoke({"messages": messages})

    # Gemini sometimes returns a list of content blocks instead of a plain string.
    # Extract only the text parts to get a clean speakable reply.
    raw = result["messages"][-1].content
    if isinstance(raw, list):
        reply = " ".join(
            block["text"] for block in raw
            if isinstance(block, dict) and block.get("type") == "text"
        ).strip()
    else:
        reply = str(raw).strip()

    # Update rolling history
    from langchain_core.messages import AIMessage
    _chat_history.append(HumanMessage(content=user_input))
    _chat_history.append(AIMessage(content=reply))
    if len(_chat_history) > _MAX_HISTORY * 2:
        _chat_history = _chat_history[-(_MAX_HISTORY * 2):]

    return reply
