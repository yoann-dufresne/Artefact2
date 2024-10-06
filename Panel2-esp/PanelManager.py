import socket
import threading
import time
from collections import deque

class PanelManagerServer:
    def __init__(self, host='0.0.0.0', port=4242):
        self.host = host
        self.port = port
        self.clients = []
        self.ping_timestamps = deque()
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.server_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Ajout de cette ligne
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        print(f"Server listening on {self.host}:{self.port}")

    def handle_client(self, client_socket, address):
        print(f"New connection from {address}")
        self.clients.append(client_socket)
        try:
            while True:
                message = client_socket.recv(1024).decode('utf-8')
                if not message:
                    break
                print(f"Received from {address}: {message}")
                if message.startswith("pong"):
                    self.handle_pong(address)
        except ConnectionResetError:
            print(f"Connection lost from {address}")
        finally:
            self.clients.remove(client_socket)
            client_socket.close()

    def handle_pong(self, address):
        # Extract the timestamp from the pong message
        if len(self.ping_timestamps) > 0:
            ping_time = self.ping_timestamps.popleft()
            pong_time = time.time()
            delay = pong_time - ping_time[1]
            print(f"Ping response from {address} received after {delay:.2f} seconds")

    def broadcast_ping(self):
        while True:
            time.sleep(10)
            timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
            ping_message = f"ping {timestamp}"
            self.ping_timestamps.append((timestamp, time.time()))
            for client in self.clients:
                try:
                    client.send(ping_message.encode('utf-8'))
                except BrokenPipeError:
                    self.clients.remove(client)

    def start(self):
        threading.Thread(target=self.broadcast_ping, daemon=True).start()
        while True:
            client_socket, address = self.server_socket.accept()
            client_thread = threading.Thread(target=self.handle_client, args=(client_socket, address))
            client_thread.start()

if __name__ == "__main__":
    server = PanelManagerServer()
    server.start()