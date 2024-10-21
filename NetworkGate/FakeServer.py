import socket
import time
import netifaces

# Adresse IP de la passerelle (par exemple 192.168.4.1) et port d'enregistrement initial
GATEWAY_IP = "192.168.4.1"
GATEWAY_PORT = 8080
SERVER_NAME = "game"
SERVER_TYPE = "server"

def get_dedicated_port():
    # Connexion initiale au port de la passerelle pour s'enregistrer
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        print(f"Connecting to gateway at {GATEWAY_IP}:{GATEWAY_PORT}...")
        sock.connect((GATEWAY_IP, GATEWAY_PORT))
        
        # Get my mac address
        mac = mac_from_socket(sock).upper()
        
        # Envoyer le message d'enregistrement
        registration_message = f"register {SERVER_NAME} {SERVER_TYPE} {mac}"
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

def mac_from_socket(sock):
    local_ip = sock.getsockname()[0]
    for interface in netifaces.interfaces():
        addrs = netifaces.ifaddresses(interface)
        if netifaces.AF_INET in addrs:
            ip_info = addrs[netifaces.AF_INET][0]
            if ip_info['addr'] == local_ip:
                mac_address = addrs[netifaces.AF_LINK][0]['addr']
                return mac_address

def start_dedicated_connection(port):
    # Connexion au port dédié pour les échanges réguliers
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        print(f"Connecting to dedicated port {port}...")
        sock.connect((GATEWAY_IP, port))
        sock.setblocking(False)

        # Envoyer des messages toutes les 5 secondes
        while True:
        # for _ in range(4):
            timestamp = int(time.time())
            message = f"server {SERVER_NAME} {timestamp}"
            print(f"Sending message: {message}")
            sock.sendall(message.encode())
            
            time.sleep(.1)
            
            # Lit les messages recus sur le socket sock
            try:
                while True:
                    data = sock.recv(1024)
                    if data:
                        print(f"Received data ({len(data)}): {data.decode('ascii')}")
                        ascii = data.decode('ascii')
                        print(f"[{', '.join([x for x in ascii])}]")
            except BlockingIOError:
                pass
            
            print()
            time.sleep(5)

if __name__ == "__main__":
    try:
        dedicated_port = get_dedicated_port()
        time.sleep(1)
        start_dedicated_connection(dedicated_port)
    except Exception as e:
        print(f"An error occurred: {e}")
