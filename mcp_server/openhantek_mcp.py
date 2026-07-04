"""MCP server for OpenHantek6022 - lets an LLM drive the oscilloscope.

Prerequisite: start OpenHantek with the remote server enabled:
    OpenHantek --server 5025

Run (needs `pip install mcp`):
    python openhantek_mcp.py

Claude Code registration:
    claude mcp add openhantek -- python /path/to/mcp_server/openhantek_mcp.py
"""

from __future__ import annotations

import os
import tempfile
import time

from mcp.server.fastmcp import FastMCP, Image

from openhantek_client import OpenHantekClient

HOST = os.environ.get("OPENHANTEK_HOST", "127.0.0.1")
PORT = int(os.environ.get("OPENHANTEK_PORT", "5025"))

mcp = FastMCP("openhantek")
client = OpenHantekClient(HOST, PORT)


@mcp.tool()
def identify() -> str:
    """Identify the connected oscilloscope (model, version)."""
    return client.idn()


@mcp.tool()
def run() -> str:
    """Start continuous acquisition."""
    return client.command("RUN") or "started"


@mcp.tool()
def stop() -> str:
    """Stop the acquisition (freeze the display)."""
    return client.command("STOP") or "stopped"


@mcp.tool()
def single() -> str:
    """Arm a single shot capture: acquire until the next trigger event, then stop."""
    return client.command("SINGLE") or "armed"


@mcp.tool()
def autoset() -> str:
    """Automatically adjust voltage gain, timebase, offset and trigger to the current signal."""
    return client.command("AUTOSET") or "done"


@mcp.tool()
def set_channel(channel: int, enable: bool | None = None, gain: float | None = None,
                coupling: str | None = None, probe: float | None = None,
                invert: bool | None = None, offset: float | None = None) -> str:
    """Configure an input channel (1 or 2).

    gain: volts per division (snapped to 20mV..10V steps), coupling: "AC" or "DC",
    probe: attenuation factor 1..1000, offset: vertical position in divisions (-4..4).
    """
    if channel not in (1, 2):
        raise ValueError("channel must be 1 or 2")
    replies = []
    if enable is not None:
        replies.append(client.command(f"CH{channel}:ENABLE {'ON' if enable else 'OFF'}"))
    if gain is not None:
        replies.append(client.command(f"CH{channel}:GAIN {gain}"))
    if coupling is not None:
        replies.append(client.command(f"CH{channel}:COUPLING {coupling.upper()}"))
    if probe is not None:
        replies.append(client.command(f"CH{channel}:PROBE {probe}"))
    if invert is not None:
        replies.append(client.command(f"CH{channel}:INVERT {'ON' if invert else 'OFF'}"))
    if offset is not None:
        replies.append(client.command(f"CH{channel}:OFFSET {offset}"))
    return "ok" if replies else "nothing to do"


@mcp.tool()
def set_timebase(seconds_per_div: float) -> str:
    """Set the horizontal timebase in seconds per division (1ns..10s, 1-2-5 steps)."""
    return client.command(f"TIMEBASE {seconds_per_div}") or "ok"


@mcp.tool()
def set_trigger(mode: str | None = None, source: int | None = None,
                slope: str | None = None, level: float | None = None) -> str:
    """Configure the trigger. mode: AUTO/NORMAL/SINGLE/ROLL, source: channel 1 or 2,
    slope: POS/NEG/BOTH, level: trigger level in volts."""
    replies = []
    if mode is not None:
        replies.append(client.command(f"TRIGGER:MODE {mode.upper()}"))
    if source is not None:
        replies.append(client.command(f"TRIGGER:SOURCE {source}"))
    if slope is not None:
        replies.append(client.command(f"TRIGGER:SLOPE {slope.upper()}"))
    if level is not None:
        replies.append(client.command(f"TRIGGER:LEVEL {level}"))
    return "ok" if replies else "nothing to do"


@mcp.tool()
def set_calibration_frequency(hertz: float) -> str:
    """Set the calibration square wave output frequency in Hz (32Hz..100kHz)."""
    return client.command(f"CALFREQ {hertz}") or "ok"


@mcp.tool()
def measure() -> dict:
    """Read the current measurements of all enabled channels:
    Vpp, Vmax, Vmin, DC, RMS, frequency."""
    return client.measure()


@mcp.tool()
def get_config() -> dict:
    """Read the complete current instrument configuration."""
    return client.config()


@mcp.tool()
def screenshot() -> Image:
    """Take a screenshot of the oscilloscope display and return it as an image."""
    path = os.path.join(tempfile.gettempdir(), f"openhantek_mcp_{int(time.time() * 1000)}.png")
    saved = client.screenshot(path)
    return Image(path=saved)


if __name__ == "__main__":
    mcp.run()
