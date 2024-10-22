
from game.enigma import *
from copy import deepcopy
import time
from pygame import mixer

class Game:

    def __init__(self, filename, gateway):
        self.enigmas = self.load_from_file(filename)
        self.reboot = False
        self.gate = gateway

        # Initialise la musique du jeu
        mixer.pre_init(buffer=4096)
        mixer.init()
        mixer.music.load("data/atmosphere.mp3")
        mixer.music.play(loops=-1)
        # Initialise le bruit du bouton
        self.button_sound = mixer.Sound("data/validation.mp3")


    def game_loop(self, log_file):
        print(f"{time.time()}\tnew game\n", file=log_file)
        print("gamelog : new game")

        for enigma in self.enigmas:
            current_enigma = deepcopy(enigma)
            print(f"{time.time()}\tnew enigma\n", file=log_file)
            print("gamelog : new enigma")

            # Envoie l'état du jeu sur le réseau de capteurs
            self.gate.send_state(current_enigma.get_state())
            while not current_enigma.is_solved():
                # Si un reboot est déclenché, quitte la game loop
                if self.reboot:
                    return
                
                btns = self.gate.button_triggered()
                if len(btns) > 0:
                    # Applique l'effet des boutons
                    for btn in self.gate.button_triggered():
                        # applique le bouton
                        is_active = current_enigma.button_triggered(btn)
                        # Joue le son du bouton si le bouton est down et qu'il est actif
                        if is_active and btn.status == Button.BUTTON_DOWN:
                            self.button_sound.play()

                    # Envoie le nouvel état du jeu
                    self.gate.send_state(current_enigma.get_state())
                
                # Vérifie si le jeu est en échec
                if current_enigma.on_error:
                    # Affiche les couleurs d'erreur pendant 3 secondes
                    time.sleep(3)
                    # Réinitialise le niveau courant
                    current_enigma = deepcopy(enigma)
                    # Envoie l'état du niveau courant
                    self.gate.send_state(current_enigma.get_state())

                time.sleep(.01)

        # Joue l'animation de fin de jeu
        # TODO

        # Attend 30 secondes avant de redémarrer le jeu
        time.sleep(30)


    def load_from_file(self, fname):
        enigmas = []
        with open(fname) as f:
            for line in f:
                line = line.strip()
                if line.startswith("#"):
                    enigmas.append(Game.parse_enigma(f))
        return enigmas


    def parse_enigma(f):
        sub_enigmas = []
        for line in f:
            line = line.strip()
            if not line:
                e = Enigma()
                [e.add_sub_enigma(se) for se in sub_enigmas]
                return e

            if line.startswith("swagLittle"):
                sub_enigmas.append(SwagLittleEnigma(line))

            elif line.startswith("swag"):
                sub_enigmas.append(SwagEnigma(line))

            elif line.startswith("button"):
                sub_enigmas.append(ButtonEnigma(line))

            elif line.startswith("little"):
                sub_enigmas.append(LittleEnigma(line))

            elif line.startswith("dark"):
                sub_enigmas.append(DarkEnigma(line))

            elif line.startswith("sequence"):
                sub_enigmas.append(SequenceEnigma(line))

