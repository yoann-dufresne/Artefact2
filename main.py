import signal
from game.Game import Game
from game.enigma import *
from game.hardware.Gateway import Gateway

import sys, os

gate = None

# Fonction de gestion du signal
def signal_handler(sig, frame):
    print("Signal reçu, fermeture de l'application...")
    global gate
    gate.stop()
    gate.join()
    sys.exit(0)

if __name__ == "__main__":
    print("Artefect version 2")
    
    # Initialise la connexion avec le réseau de capteurs du jeu
    gate = Gateway()
    # Initialise le jeu
    game = Game(sys.argv[1], gate)
    
    signal.signal(signal.SIGINT, signal_handler)

    # Open log files
    nb_logs_in_dir = len(os.listdir("./game_log"))
    game_log_file = "./game_log/{}.log".format(nb_logs_in_dir)
    with open(game_log_file, "w") as f:
        # Loop over the full game
        while True:
            game.game_loop(f)

