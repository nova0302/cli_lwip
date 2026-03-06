# tc2_cli.py --- 
# 
# Filename: tc2_cli.py
# Description: 
# Author: Sanglae Kim
# Maintainer: 
# Created: Thu Mar  5 18:31:43 2026 (+0900)
# Version: 
# Package-Requires: ()
# Last-Updated: Fri Mar  6 11:01:50 2026 (+0900)
#           By: Sanglae Kim
#     Update #: 5
# URL: 
# Doc URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# 
# 
# 

# Change Log:
# 
# 
# 
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.
# 
# 

# Code:
import socket
import json
import sys
import argparse
import os
import base64
import re
import threading
import queue

# Try to import readline (standard on Linux/macOS)
try:
    import readline
except ImportError:
    readline = None

#HEX64_RE = re.compile(r'^[0-9a-fA-F]{128}$')  # 128 hex chars == 64 bytes
HEX256_RE = re.compile(r'^[0-9a-fA-F]{512}$')  # 512 hex chars == 256 bytes

class TCPClient:
    def __init__(self, host, port, rx_timeout=10.0):
        self.host = host
        self.port = port
        self.sock = None
        self.history_file = os.path.expanduser("~/.hw_client_history")
        self.rx_timeout = rx_timeout

        # --- async receive machinery ---
        self._rx_thread = None
        self._rx_stop = threading.Event()
        self._rx_queue: "queue.Queue[object]" = queue.Queue()
        self._rx_buf = ""   # text buffer for newline framing

        # Command schema: (arg_count, help_text)
        self.cmd_schema = {
            "reade" : (2, "reade <addr> <bytes>"),
            "readb" : (2, "readb <addr> <bytes>"),
            "readva": (2, "readva <addr> <nVfyData>"),
            # For apgm/dpgm, second arg is @binfile OR 128-hex-chars; we send 64 bytes (base64)
            "apgm"  : (2, "apgm <addr> <@binfile|hex512>  # sends EXACTLY 256 bytes"),
            "dpgm"  : (2, "dpgm <addr> <@binfile|hex512>  # sends EXACTLY 256 bytes"),
            "bpgm"  : (2, "bpgm <addr> <value>"),
            "erase" : (1, "erase <sector_id>"),
            "ce"    : (1, "ce [on/off]"),
            "por"   : (0, "por"),
            "h"     : (0, "help"),
            "i"     : (0, "init")
        }
        self._setup_readline()

    # -------------------------
    # readline / CLI helpers
    # -------------------------
    def _setup_readline(self):
        """Initializes command history and auto-completion."""
        if readline:
            if os.path.exists(self.history_file):
                try:
                    readline.read_history_file(self.history_file)
                except Exception:
                    pass

            def completer(text, state):
                options = [c for c in self.cmd_schema.keys() if c.startswith(text)]
                return options[state] if state < len(options) else None

            readline.set_completer(completer)
            readline.parse_and_bind("tab: complete")

    # -------------------------
    # socket connect / threads
    # -------------------------
    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # blocking socket is fine; RX thread reads in background
            self.sock.settimeout(10)
            self.sock.connect((self.host, self.port))
            print(f"[*] Connected to {self.host}:{self.port}")
        except Exception as e:
            print(f"[!] Connection error: {e}")
            sys.exit(1)

        # Start async receiver thread once connected
        self._rx_stop.clear()
        self._rx_thread = threading.Thread(target=self._recv_loop, name="tcp-recv", daemon=True)
        self._rx_thread.start()

    def close(self):
        try:
            self._rx_stop.set()
            if self.sock:
                try:
                    self.sock.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                self.sock.close()
                self.sock = None
            if self._rx_thread and self._rx_thread.is_alive():
                self._rx_thread.join(timeout=1.0)
        except Exception:
            pass

    # -------------------------
    # async receive loop
    # -------------------------
    def _recv_loop(self):
        """Read newline-delimited messages, parse JSON if possible, push to queue."""
        try:
            while not self._rx_stop.is_set():
                try:
                    data = self.sock.recv(4096)
                    if not data:
                        # socket closed by peer
                        self._rx_queue.put({"error": "Connection closed by peer"})
                        break
                    self._rx_buf += data.decode('utf-8', errors='replace')
                    # Split into complete lines
                    while True:
                        if '\n' not in self._rx_buf:
                            break
                        line, self._rx_buf = self._rx_buf.split('\n', 1)
                        line = line.strip()
                        if not line:
                            continue
                        # Try parse JSON; if fails, keep raw text
                        try:
                            msg = json.loads(line)
                        except json.JSONDecodeError:
                            msg = line
                        self._rx_queue.put(msg)
                except socket.timeout:
                    continue
                except Exception as e:
                    self._rx_queue.put({"error": f"Receive error: {e}"})
                    break
        finally:
            self._rx_stop.set()

    def _await_response(self, timeout=None):
        """Wait for one message from the RX queue."""
        if timeout is None:
            timeout = self.rx_timeout
        try:
            return self._rx_queue.get(timeout=timeout)
        except queue.Empty:
            return {"error": f"Network Error: timed out after {timeout:.1f}s"}

    # -------------------------
    # base64 helpers (64-byte)
    # -------------------------
    @staticmethod
    def _read_256_from_file(path):
        path = path[1:] if path.startswith('@') else path
        with open(path, 'rb') as f:
            data = f.read(256)
        if len(data) != 256:
            raise ValueError(f"File '{path}' has {len(data)} bytes (need exactly 256)")
        return data

    @staticmethod
    def _hex_to_bytes_256(hexstr):
        if not HEX256_RE.match(hexstr):
            raise ValueError("hex payload must be exactly 512 hex characters (256 bytes)")
        # Convert pairs into bytes
        return bytes(int(hexstr[i:i+2], 16) for i in range(0, 512, 2))

    def _prepare_b64_256(self, src_token):
        """
        src_token: '@file' OR 128-hex-chars.
        Returns base64 string for exactly 64 bytes.
        """
        if src_token.startswith('@'):
            raw = self._read_256_from_file(src_token)
        else:
            raw = self._hex_to_bytes_256(src_token)
        return base64.b64encode(raw).decode('ascii')

    # -------------------------
    # command handling
    # -------------------------
    def execute_command(self, user_input):
        parts = user_input.split()
        if not parts:
            return

        cmd_name = parts[0].lower()
        args = parts[1:]

        if cmd_name not in self.cmd_schema:
            print(f"[!] Unknown command: {cmd_name}")
            return

        required_args, usage = self.cmd_schema[cmd_name]
        if len(args) != required_args:
            print(f"[!] Usage: {usage}")
            return

        try:
            # Special handling for apgm/dpgm with 64-byte payload
            if cmd_name in ("apgm", "dpgm"):
                addr, src_token = args[0], args[1]
                data_b64 = self._prepare_b64_256(src_token)
                payload = {"cmd": cmd_name, "args": [addr, "256"], "data_b64": data_b64}
            else:
                payload = {"cmd": cmd_name, "args": args}
        except Exception as e:
            print(f"[!] Payload error: {e}")
            return

        print(f"tx : {payload}")
        ok = self._send_json(payload)
        if not ok:
            print("[!] Send failed")
            return

        # Wait for one response (async RX thread fills the queue)
        resp = self._await_response()
        print("rx:")
        print(json.dumps(resp, indent=2) if isinstance(resp, dict) else resp)

    def _send_json(self, data):
        try:
            message = (json.dumps(data) + "\n").encode('utf-8')
            self.sock.sendall(message)
            return True
        except Exception as e:
            print(f"[!] Send error: {e}")
            return False

    # -------------------------
    # interactive CLI loop
    # -------------------------
    def run_cli(self):
        print("\n--- Hardware Control Interface ---")
        print(f"Commands: {', '.join(self.cmd_schema.keys())}")
        print("Use UP/DOWN arrows for history, TAB for completion.")
        print("For apgm/dpgm payload: use '@path/to/file.bin' OR 128-hex-chars (64 bytes).\n")

        try:
            while True:
                user_input = input("tc2[b]> ").strip()
                if user_input.lower() in ['exit', 'q']:
                    break
                if not user_input:
                    continue
                if user_input in ("lh", "help"):
                    for k, v in self.cmd_schema.items():
                        print(f"  {k:6s} : {v[1]}")
                    continue
                self.execute_command(user_input)
        except (KeyboardInterrupt, EOFError):
            print("\nExiting...")
        finally:
            if readline:
                try:
                    readline.write_history_file(self.history_file)
                except Exception:
                    pass
            self.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("host", nargs='?', default="192.168.123.10")
    parser.add_argument("port", nargs='?', type=int, default=7)
    parser.add_argument("--timeout", type=float, default=10.0,
                        help="Response wait timeout (seconds)")
    cl_args = parser.parse_args()

    client = TCPClient(cl_args.host, cl_args.port, rx_timeout=cl_args.timeout)
    client.connect()
    client.run_cli()

# 
# tc2_cli.py ends here
