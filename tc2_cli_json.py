# tc2_cli_json.py --- 
# 
# Filename: tc2_cli_json.py
# Description: 
# Author: Sanglae Kim
# Maintainer: 
# Created: Fri Mar  6 14:42:28 2026 (+0900)
# Version: 
# Package-Requires: ()
# Last-Updated: Fri Mar 20 17:53:22 2026 (+0900)
#           By: Sanglae Kim
#     Update #: 44
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

# --- Protocol defaults & constraints for APGM/DPGM ---
DEFAULT_NXADDR = 0           # 0 .. 1280
DEFAULT_NVCG   = 0xDEF       # (4.32[V] assumed default in allowed range 0x50F .. 0xFFF
DEFAULT_REPEAT = 1           # 1 .. 64
DEFAULT_FILE   = "file256.bin"
PAYLOAD_BYTES  = 256         # 256 * 8 = 2048 bit lines
HEX_LEN        = PAYLOAD_BYTES * 2
HEX256_RE      = re.compile(r'^[0-9a-fA-F]{' + str(HEX_LEN) + r'}$')

def b64_to_bytes(b64str):
    try:
        return base64.b64decode(b64str)
    except Exception as e:
        print(f"[!] Base64 decode error: {e}")
        return None

def _int_from_token(tok: str) -> int:
    """Parse token as int; accepts decimal or 0x... hex."""
    return int(tok, 0)

def _normalize_hex(v: int) -> str:
    """Return 0x uppercase hex (no leading zeros beyond what's needed)."""
    return f"0x{v:X}"

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

        # Command schema: (min_args, max_args, help_text)
        self.cmd_schema = {
            # unchanged commands
            "readn" : (0, 3, "readn [nrTagStart=0] [nrTagNum=1] [nrReg=0xFF]"),
            "reade" : (0, 2, "reade [nxAddr=0] [MAIN/NVR/ARRDN/ARRCOL]"),
            # "reade" : (2, 2, "reade <addr> <bytes>"),
            "readb" : (2, 2, "readb <addr> <bytes>"),
            #"readva": (2, 2, "readva [nxAddr:0] [nVfyData]"),
            #"readvb": (2, 2, "readvb [nxAddr:0] [nVfyData]"),
            "dpgm"  : (2, 2, "dpgm [nxAddr:0] [MAIN/NVR/ARRDN/ARRCOL]"),
            "apgm"  : (2, 2, "apgm [nxAddr:0] [nVcg] [MAIN/NVR/ARRDN/ARRCOL]"),
            "erase" : (0, 2, "erase [nNumSec:0] [nNumToErase:1] (defaults: 0, 1)"),
            "ce"    : (1, 1, "ce [on/off]"),
            "por"   : (0, 0, "por"),
            "h"     : (0, 0, "help"),
            "i"     : (0, 0, "init"),

            # NEW: apgm/dpgm accept 0..4 args (nxAddr, nVcg, nNumRepeat, pgmData)
            # pgmData: '@file' or 512-hex-chars; default file256.bin
            "apgm"  : (0, 4, "apgm [nxAddr] [nVcg] [nNumRepeat] [@file|hex512]  (defaults: 0, 0x50F, 1, file256.bin)"),
            "dpgm"  : (0, 4, "dpgm [nxAddr] [nVcg] [nNumRepeat] [@file|hex512]  (defaults: 0, 0x50F, 1, file256.bin)"),
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
    # base64 helpers (256-byte)
    # -------------------------
    @staticmethod
    def _read_n_from_file(path, n=PAYLOAD_BYTES):
        with open(path, 'rb') as f:
            data = f.read(n)
        if len(data) != n:
            raise ValueError(f"File '{path}' has {len(data)} bytes (need exactly {n})")
        return data

    @staticmethod
    def _hex_to_bytes(hexstr, nbytes=PAYLOAD_BYTES):
        expected = nbytes * 2
        if not re.fullmatch(r'[0-9a-fA-F]{' + str(expected) + r'}', hexstr or ''):
            raise ValueError(f"hex payload must be exactly {expected} hex characters ({nbytes} bytes)")
        return bytes(int(hexstr[i:i+2], 16) for i in range(0, expected, 2))

    def _prepare_b64_payload(self, src_token: str | None) -> str:
        """
        src_token:
          - None -> default file DEFAULT_FILE
          - string starting with '@' -> file path (first 256 bytes)
          - 512-hex-chars -> exact 256 bytes
        Returns base64-encoded ASCII string for exactly 256 bytes.
        """
        if not src_token:
            raw = self._read_n_from_file(DEFAULT_FILE, PAYLOAD_BYTES)
        elif src_token.startswith('@'):
            path = src_token[1:]  # strip '@'
            if not path:
                path = DEFAULT_FILE
            raw = self._read_n_from_file(path, PAYLOAD_BYTES)
        else:
            raw = self._hex_to_bytes(src_token, PAYLOAD_BYTES)
        return base64.b64encode(raw).decode('ascii')

    def _parse_reade(self, args):
        # 1) nxAddr
        if len(args) >= 1:
            start_val = _int_from_token(args[0])
        else:
            start_val = 0
        if not (0 <= start_val <= 1279):
            raise ValueError("nxAddr out of range")

        return str(start_val)
 
    def _parse_readn(self, args):
        # 1) nrTagStart
        if len(args) >= 1:
            start_val = _int_from_token(args[0])
        else:
            start_val = 0
        if not (0 <= start_val <= 0xFFFF):
            raise ValueError("nrTagStart out of range")

        # 2) nrTagNum
        if len(args) >= 2:
            num_val = _int_from_token(args[1])
        else:
            num_val = 0
        if not (0 <= num_val <= 128):
            raise ValueError("nrTagNum out of range (1..128)")

        # 3) nrReg
        if len(args) >= 3:
            reg_val = _int_from_token(args[2])
        else:
            reg_val = 0xFF
        if not (0 <= reg_val <= 0xFF):
            raise ValueError("nrReg out of range")

        return str(start_val), str(num_val), f"0x{reg_val:X}"
    # -------------------------
    # APGM/DPGM argument parsing (0..4 args)
    # -------------------------
    def _parse_apgm_like(self, args):
        """
        Returns a tuple (nxAddr_str, nVcg_str, nRepeat_str, pgmData_b64)
        with validation and defaults applied; all returned as strings (except pgmData as b64).
        """
        # 1) nxAddr
        if len(args) >= 1:
            nx_addr_val = _int_from_token(args[0])
        else:
            nx_addr_val = DEFAULT_NXADDR
        if not (0 <= nx_addr_val <= 1280):
            raise ValueError("nxAddr out of range (0..1280)")
        nx_addr_str = str(nx_addr_val)  # send as decimal string

        # 2) nVcg
        if len(args) >= 2:
            nvcg_val = _int_from_token(args[1])
        else:
            nvcg_val = DEFAULT_NVCG
        if not (0x50F <= nvcg_val <= 0xFFF):
            raise ValueError("nVcg out of range (0x50F..0xFFF)")
        nvcg_str = _normalize_hex(nvcg_val)  # send as hex string "0x..."

        # 3) nNumRepeat
        if len(args) >= 3:
            repeat_val = _int_from_token(args[2])
        else:
            repeat_val = DEFAULT_REPEAT
        if not (1 <= repeat_val <= 64):
            raise ValueError("nNumRepeat out of range (1..64)")
        repeat_str = str(repeat_val)  # send as decimal string

        # 4) pgmData (optional)
        pgm_token = args[3] if len(args) >= 4 else None
        pgm_b64 = self._prepare_b64_payload(pgm_token)

        return nx_addr_str, nvcg_str, repeat_str, pgm_b64
    # -------------------------
    # ERASE argument parsing (0..2 args)
    # -------------------------
    def _parse_erase_like(self, args):
        """
        Returns a tuple (nSectNumber, nNumToErase)
        with validation and defaults applied; all returned as strings.
        """
        # 1) nxAddr
        if len(args) >= 1:
            nx_addr_val = _int_from_token(args[0])
        else:
            nx_addr_val = DEFAULT_NXADDR
        if not (0 <= nx_addr_val <= 1280):
            raise ValueError("nxAddr out of range (0..1280)")
        nx_addr_str = str(nx_addr_val)  # send as decimal string

        # 2) nVcg
        if len(args) >= 2:
            nvcg_val = _int_from_token(args[1])
        else:
            nvcg_val = 1
        if not (0 <= nvcg_val <= 1280):
            raise ValueError("nVcg out of range (0..1280)")
        nvcg_str = _normalize_hex(nvcg_val)  # send as hex string "0x..."

        return nx_addr_str, nvcg_str
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

        min_args, max_args, usage = self.cmd_schema[cmd_name]
        if not (min_args <= len(args) <= max_args):
            print(f"[!] Usage: {usage}")
            return

        try:
            if cmd_name in ("apgm", "dpgm"):
                nx_addr_str, nvcg_str, repeat_str, pgm_b64 = self._parse_apgm_like(args)
                # Keep args as strings, and put payload in "pgmData" (base64)
                payload = {
                    "cmd": cmd_name,
                    "args": [nx_addr_str, nvcg_str, repeat_str],
                    "pgmData": pgm_b64,
                    # optional: send payload length explicitly for sanity
                    "payloadLen": str(PAYLOAD_BYTES)
                }
            elif cmd_name == "reade":
                nxaddr_str = self._parse_reade(args)
                payload = {
                    "cmd": "reade",
                    "args": [nxaddr_str]
                }
            elif cmd_name == "readn":
                start_str, num_str, reg_str = self._parse_readn(args)
                payload = {
                    "cmd": "readn",
                    "args": [start_str, num_str, reg_str]
                }
            elif cmd_name == "erase":
                nx_addr_str, nvcg_str = self._parse_erase_like(args)
                payload = {
                    "cmd": cmd_name,
                    "args": [nx_addr_str, nvcg_str]
                }
            else:
                # default pass-through
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
        print(f"rx : {resp}")

        # If not JSON object, just print it
        if not isinstance(resp, dict):
            print(resp)
            return

        cmd = resp.get("cmd", "")
        payload = resp.get("payload")

        # Special handling for READN
        if cmd == "readn" and payload:
            raw = b64_to_bytes(payload)
            if raw is None:
                print(f"[!] Failed to decode {cmd} payload")
                return

            # Pretty hex dump
            print(raw.hex())

            # Convert to list of uint8
            data = list(raw)

            # Remove last 2 bytes
            if len(data) >= 2:
                data = data[:-2]

            print(f"[readn] received {len(data)} valid bytes")

            # ---- DRAW GRAPH ----
            try:
                import matplotlib.pyplot as plt

                # X axis: 0 ~ len(data)-1 (expected 2047)
                x = list(range(len(data)))

                plt.figure(figsize=(10, 4))
                plt.plot(x, data, marker='.', linestyle='-', color='blue')
                plt.xlabel("Index (0~2047)")
                plt.ylabel("Value (uint8)")
                plt.title("readn Data Plot")
                plt.grid(True)
                plt.tight_layout()
                plt.show()

            except Exception as e:
                print(f"[!] Graph error: {e}")

            return

        elif cmd == "reade" and payload:
            raw = b64_to_bytes(payload)
            if raw is None:
                print(f"[!] Failed to decode {cmd} payload")
                return
            print(f"[{cmd}] received {len(raw)} bytes")
            # Pretty hex dump
            print(raw.hex())
            return

        # Default printing for other commands
        print(json.dumps(resp, indent=2))

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
        print("Commands:", ", ".join(self.cmd_schema.keys()))
        print("Use UP/DOWN arrows for history, TAB for completion.")
        print("\nAPGM/DPGM usage:")
        print("  apgm [nxAddr] [nVcg] [nNumRepeat] [@file|hex512]")
        print(f"  defaults -> nxAddr={DEFAULT_NXADDR}, nVcg={_normalize_hex(DEFAULT_NVCG)}, nNumRepeat={DEFAULT_REPEAT}, file={DEFAULT_FILE}")
        print(f"  payload size = {PAYLOAD_BYTES} bytes (Base64 in JSON key 'pgmData')\n")

        try:
            while True:
                user_input = input("tc2[b]> ").strip()
                if user_input.lower() in ['exit', 'q']:
                    break
                if not user_input:
                    continue
                if user_input in ("hh", "help"):
                    for k, v in self.cmd_schema.items():
                        print(f"  {k:6s} : {v[2]}")
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
# tc2_cli_json.py ends here
