import socket
import time
import signal
import sys

# Configuration
ESP32_IP = '192.168.4.1'  # Adresse IP de l'ESP32
PORT = 4000               # Port auquel se connecter
PING_MESSAGE = "ping"      # Message à envoyer
BUFFER_SIZE = 128          # Taille du tampon pour la réponse

# Initialisation du socket
sock = None

def connect_to_esp():
    global sock
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ESP32_IP, PORT))
        print(f"Connecté à {ESP32_IP}:{PORT}")
    except Exception as e:
        print(f"Erreur de connexion: {e}")
        sys.exit(1)

def send_ping_and_receive_pong():
    try:
        # Envoi du message "ping"
        start_time = time.time()  # Temps de départ
        sock.sendall(PING_MESSAGE.encode('ascii'))
        print(f"Message envoyé: {PING_MESSAGE}")
        
        # Réception de la réponse "pong"
        response = sock.recv(BUFFER_SIZE).decode('ascii')
        if response == "pong":
            end_time = time.time()  # Temps de fin
            elapsed_time_ms = (end_time - start_time) * 1000  # Conversion en millisecondes
            print(f"Réponse reçue: {response} (temps écoulé: {elapsed_time_ms:.2f} ms)")
        else:
            print(f"Réponse inattendue reçue: {response}")
    
    except Exception as e:
        print(f"Erreur d'envoi/réception: {e}")
        sys.exit(1)

def close_socket(signal_received=None, frame=None):
    global sock
    if sock:
        print("Fermeture de la connexion...")
        sock.close()
    print("Programme terminé.")
    sys.exit(0)

# Gestion du signal d'interruption (Ctrl+C)
signal.signal(signal.SIGINT, close_socket)

def main():
    connect_to_esp()
    while True:
        send_ping_and_receive_pong()
        time.sleep(2)  # Attendre 2 secondes avant le prochain "ping"

if __name__ == "__main__":
    main()
