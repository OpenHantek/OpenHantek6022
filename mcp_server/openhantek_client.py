"""Minimal TCP client for the OpenHantek6022 remote control server.

Start OpenHantek with:  OpenHantek --server 5025
The protocol is line based, SCPI style; replies start with "OK" or "ERR".
"""

from __future__ import annotations

import json
import socket


class OpenHantekError(RuntimeError):
    pass


class OpenHantekClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 5025, timeout: float = 5.0):
        self.host = host
        self.port = port
        self.timeout = timeout

    def command(self, line: str) -> str:
        """Send one command line, return the reply payload (without the OK prefix)."""
        with socket.create_connection((self.host, self.port), timeout=self.timeout) as sock:
            sock.sendall(line.strip().encode() + b"\n")
            reply = b""
            while not reply.endswith(b"\n"):
                chunk = sock.recv(65536)
                if not chunk:
                    break
                reply += chunk
        text = reply.decode().strip()
        if text.startswith("ERR"):
            raise OpenHantekError(text[3:].strip())
        if text.startswith("OK"):
            return text[2:].strip()
        return text  # e.g. *IDN? replies without OK prefix

    def command_json(self, line: str) -> dict:
        return json.loads(self.command(line))

    # convenience wrappers
    def idn(self) -> str:
        return self.command("*IDN?")

    def measure(self) -> dict:
        return self.command_json("MEASURE?")

    def config(self) -> dict:
        return self.command_json("CONFIG?")

    def screenshot(self, path: str = "") -> str:
        return self.command(f"SCREENSHOT {path}".strip())
