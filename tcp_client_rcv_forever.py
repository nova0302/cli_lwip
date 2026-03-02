import socket
import time

HOST = '192.168.0.10'
PORT = 7
BUFFER_SIZE = 1024  # Small buffer size for demonstration

def start_tcp_client(host, port):
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print(f"Attempting to connect to {host}:{port}...")
                s.connect((host, port))
                print("Connection established. Waiting for data...")
                s.sendall(b'Hello, Znyq')

                while True:
                    # Receive data in a loop
                    data = s.recv(BUFFER_SIZE)
                    if not data:
                        # If recv returns 0 bytes, the server has closed the connection
                        print("Server closed the connection. Reconnecting...")
                        break
                    print(f"Received: {data.decode('utf-8')}")

        except ConnectionRefusedError:
            print("Connection refused. Make sure the server is running.")
        except Exception as e:
            print(f"An error occurred: {e}")
        
        # Wait before attempting to reconnect
        print("Waiting 5 seconds before retrying connection...")
        time.sleep(5)

if __name__ == "__main__":
    start_tcp_client(HOST, PORT)
