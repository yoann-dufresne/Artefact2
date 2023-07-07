from game.ArtefactServer import ArtefactServer
from game.enigma import *
from game.Game import Game, game_loop
from game.hardware.real import Device as RealDevice

import sys, os


if __name__ == "__main__":
    print("Artefect version 2")
    device = RealDevice()

    enigmas = Game.load_from_file(sys.argv[1])
    # for enigma in enigmas:
    #     print(enigma.sub_enigmas)
    # exit(0)
    nb_logs_in_dir = len(os.listdir("./game_log"))
    game_log_file = "./game_log/{}.log".format(nb_logs_in_dir)
    with open(game_log_file, "w") as f:
        # device.log_file = f
        game_loop(device, enigmas, f)

