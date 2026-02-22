import socket
import threading
import matplotlib.pyplot as plt
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.button import Button
from kivy_garden.matplotlib.backend_kivyagg import FigureCanvasKivyAgg

class GraphApp(App):
    def build(self):
        root = BoxLayout(orientation='horizontal', padding=10, spacing=10)

        # --- Sidebar with Buttons ---
        sidebar = BoxLayout(orientation='vertical', size_hint=(0.2, 1), spacing=10)
        
        btn_connect = Button(text="Connect TCP", size_hint=(1, 0.2))
        # Bind the button to our threaded function
        btn_connect.bind(on_release=self.start_tcp_thread)
        
        btn_exit = Button(text="Exit", size_hint=(1, 0.2))
        btn_exit.bind(on_release=self.stop)

        sidebar.add_widget(btn_connect)
        sidebar.add_widget(btn_exit)
        root.add_widget(sidebar)

        # --- Graphs ---
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(5, 8))
        self.fig.tight_layout(pad=4.0)
        self.canvas = FigureCanvasKivyAgg(self.fig)
        root.add_widget(self.canvas)
        
        return root

    def start_tcp_thread(self, instance):
        # Start TCP work in background so UI doesn't freeze
        threading.Thread(target=self.run_tcp_client, daemon=True).start()

    def run_tcp_client(self):
        host = '192.168.0.10'  # Change to your server IP
        port = 7        # Change to your server Port
        
        try:
            print(f"Connecting to {host}...")
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5) # Don't wait forever
                s.connect((host, port))
                s.sendall(b"Hello Server")
                data = s.recv(1024)
                print(f"Received from server: {data.decode()}")
                
                # Logic to update graph data could go here
                
        except Exception as e:
            print(f"TCP Error: {e}")

if __name__ == '__main__':
    GraphApp().run()
