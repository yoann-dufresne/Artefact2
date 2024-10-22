import errno
import socket
from threading import Thread
import time
import netifaces


class Gateway(Thread):

    def __init__(self):
        super().__init__()
        
        self.running = True
        self.mailbox = []
        
        self.last_contact = time.time()
        self.start()
        
    def stop(self):
        self.running = False

    def send_state(self, state):
        self.mailbox.extend(state.octopus_messages())
        self.mailbox.extend(state.panel_messages())

    def button_triggered(self):
        return []


    def run(self):
        while self.running:
            try:
                # Connection au socket
                sock = self.connect()
            except (TimeoutError, ValueError) as e:
                print("Connection timeout, retrying in 1s...")
                time.sleep(1)
                continue
            except OSError as e:
                if e.errno in [errno.EHOSTUNREACH, errno.ENETUNREACH]:
                    print("Host unreachable, retrying in 1s...")
                    time.sleep(1)
                    continue
                raise

            while self.running:
                try:
                    # Envoie les messages
                    self.send_waiting_msgs(sock)

                    # Récupère les message
                    self.receive_msgs(sock)
                    
                except (BrokenPipeError, ConnectionResetError, OSError) as e:
                    print("Erreur de connexion...")
                    # Sort de la boucle interne pour retenter une connexion
                    break

                time.sleep(.01)

            # fermeture du socket
            
    def send_waiting_msgs(self, socket):
        for message in self.mailbox:
            time.sleep(.001)
            socket.sendall((message + '\n').encode())
        self.mailbox = []
            
    def receive_msgs(self, sock):
        # Lit les messages recus sur le socket sock
        try:
            data = sock.recv(1024)
            if data:
                print(f"Received data ({len(data)}): {data.decode('ascii')}")
                ascii = data.decode('ascii')
                print(f"[{', '.join([x for x in ascii])}]")
        except BlockingIOError:
            pass
        
        # Envoi d'un message de "keep-alive" pour vérifier la connexion
        try:
            if time.time() - self.last_contact > 1.0:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as test_sock:
                    test_sock.connect(("192.168.4.1", 8080))
                self.last_contact = time.time()
        except (BrokenPipeError, ConnectionResetError, OSError) as e:
            print(f"Connection perdue: {e}")
            raise
            

    def connect(self):
        port = self.get_dedicated_port()
        # Attend l'établissement de la connexion par le nouveau port côté ESP32
        time.sleep(.1)
        sock = self.start_dedicated_connection(port)
        return sock

    def get_dedicated_port(self):
        # Connexion initiale au port de la passerelle pour s'enregistrer
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.settimeout(1)
            print(f"Connecting to gateway at 192.168.4.1:8080...")
            sock.connect(("192.168.4.1", 8080))
            
            # Get my mac address
            mac = mac_from_socket(sock).upper()
            
            # Envoyer le message d'enregistrement
            registration_message = f"register game server {mac}"
            print(f"Sending registration message: {registration_message}")
            sock.sendall(registration_message.encode())

            # Recevoir le port dédié pour les échanges futurs
            response = sock.recv(1024).decode()
            print(f"Received response: {response}")
            
            # Extraire le numéro de port
            if response.startswith("port"):
                _, port = response.split()
                return int(port)
            else:
                raise ValueError("Invalid response from gateway")

    def start_dedicated_connection(self, port):
        # ouvre un socket bidirectionnel vers la passerelle
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.settimeout(1)
        
        # Connexion au port dédié pour les échanges réguliers
        print(f"Connecting to dedicated port {port}...")
        sock.connect(("192.168.4.1", port))
        sock.setblocking(False)
        
        return sock


def mac_from_socket(sock):
    local_ip = sock.getsockname()[0]
    for interface in netifaces.interfaces():
        addrs = netifaces.ifaddresses(interface)
        if netifaces.AF_INET in addrs:
            ip_info = addrs[netifaces.AF_INET][0]
            if ip_info['addr'] == local_ip:
                mac_address = addrs[netifaces.AF_LINK][0]['addr']
                return mac_address