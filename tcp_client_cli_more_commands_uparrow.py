import socket
import json
import sys
import argparse
import os

# Try to import readline (standard on Linux/macOS)
# For Windows, you may need: pip install pyreadline3
try:
    import readline
except ImportError:
    readline = None

class TCPClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.history_file = os.path.expanduser("~/.hw_client_history")
        
        # Command schema: (arg_count, help_text)
        self.cmd_schema = {
            "reade": (2, "reade <addr> <bytes>"),
            "readb": (2, "readb <addr> <bytes>"),
            "readv": (2, "readv <addr> <bytes>"),
            "apgm":  (2, "apgm <addr> <value>"),
            "dpgm":  (2, "dpgm <addr> <value>"),
            "bpgm":  (2, "bpgm <addr> <value>"),
            "erase": (1, "erase <sector_id>"),
            "por":   (0, "por")
        }
        self._setup_readline()

    def _setup_readline(self):
        """Initializes command history and auto-completion."""
        if readline:
            # Load existing history if it exists
            if os.path.exists(self.history_file):
                readline.read_history_file(self.history_file)
            
            # Set up tab-completion for your specific commands
            def completer(text, state):
                options = [c for c in self.cmd_schema.keys() if c.startswith(text)]
                return options[state] if state < len(options) else None

            readline.set_completer(completer)
            readline.parse_and_bind("tab: complete")

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(10)
            self.sock.connect((self.host, self.port))
            print(f"[*] Connected to {self.host}:{self.port}")
        except Exception as e:
            print(f"[!] Connection error: {e}")
            sys.exit(1)

    def execute_command(self, user_input):
        parts = user_input.split()
        if not parts: return
        
        cmd_name = parts[0].lower()
        args = parts[1:]

        if cmd_name not in self.cmd_schema:
            print(f"[!] Unknown command: {cmd_name}")
            return

        required_args, usage = self.cmd_schema[cmd_name]
        if len(args) != required_args:
            print(f"[!] Usage: {usage}")
            return

        payload = {"cmd": cmd_name, "args": args}
        return self._send_json(payload)

    def _send_json(self, data):
        try:
            message = (json.dumps(data) + "\n").encode('utf-8')
            self.sock.sendall(message)
            response = self.sock.recv(4096).decode('utf-8')
            try:
                return json.loads(response)
            except json.JSONDecodeError:
                return response
        except Exception as e:
            return f"Network Error: {e}"

    def run_cli(self):
        print("\n--- Hardware Control Interface ---")
        print(f"Commands: {', '.join(self.cmd_schema.keys())}")
        print("Use UP/DOWN arrows for history, TAB for completion.\n")
        
        try:
            while True:
                # input() now automatically uses the readline buffer
                user_input = input("HW_CTRL > ").strip()
                
                if user_input.lower() in ['exit', 'quit']:
                    break
                if not user_input:
                    continue
                
                result = self.execute_command(user_input)
                if result:
                    print(json.dumps(result, indent=2) if isinstance(result, dict) else result)
        except (KeyboardInterrupt, EOFError):
            print("\nExiting...")
        finally:
            if readline:
                readline.write_history_file(self.history_file)
            if self.sock: 
                self.sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("host", nargs='?', default="127.0.0.1")
    parser.add_argument("port", nargs='?', type=int, default=65432)
    cl_args = parser.parse_args()

    client = TCPClient(cl_args.host, cl_args.port)
    client.connect()
    client.run_cli()
