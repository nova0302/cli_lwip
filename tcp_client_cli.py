import socket
import argparse
import sys

class TCPClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"[*] Connected to {self.host}:{self.port}")
        except Exception as e:
            print(f"[!] Error: {e}")
            sys.exit(1)

    def execute_command(self, user_input):
        """Parses input and handles command logic."""
        parts = user_input.split()
        if not parts:
            return

        cmd = parts[0].lower()

        # Custom logic for 'read' command: read <addr> <nBytes>
        if cmd == "read" and len(parts) == 3:
            addr, n_bytes = parts[1], parts[2]
            payload = f"READ_REQ|{addr}|{n_bytes}"
            return self._send_raw(payload)
        
        # Default behavior for other inputs
        return self._send_raw(user_input)

    def _send_raw(self, data):
        try:
            self.sock.sendall(data.encode('utf-8'))
            return self.sock.recv(4096).decode('utf-8')
        except Exception as e:
            return f"Network Error: {e}"

    def run_cli(self):
        print("\n--- TCP Interactive Shell ---")
        print("Commands: read <addr> <nBytes> | exit")
        try:
            while True:
                # The 'Prompt' for the user
                cmd_input = input("Command > ").strip()
                
                if cmd_input.lower() in ['exit', 'quit']:
                    break
                
                if cmd_input:
                    response = self.execute_command(cmd_input)
                    print(f"[Server Response]: {response}")
        except KeyboardInterrupt:
            pass
        finally:
            self.sock.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("port", type=int)
    args = parser.parse_args()

    client = TCPClient(args.host, args.port)
    client.connect()
    client.run_cli()
