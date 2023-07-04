"""Main de test."""

import sys
import os
import time
import logging

from game.hardware.debug import Device as DebugDevice
from game.hardware.real import Device as RealDevice

from game.enigma import *
from copy import deepcopy
import time


logger = logging.getLogger('root')
FORMAT = (
    '[%(asctime)s :: %(levelname)s '
    '%(filename)s:%(lineno)s - %(funcName)10s() ]'
    ' :: %(message)s'
)

logging.basicConfig(format=FORMAT)
#setup_logging(handler)
logger.setLevel(logging.CRITICAL)

try:
    os.mkdir("./game_log")
except FileExistsError:
    pass


class Game:

    @classmethod
    def parse_enigma(cls, f):
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


    @classmethod
    def load_from_file(self, fname):
        enigmas = []
        with open(fname) as f:
            for line in f:
                line = line.strip()
                if line.startswith("#"):
                    enigmas.append(self.parse_enigma(f))
        return enigmas


def game_loop(device, enigmas, log_file):
    log_file.write("{}\t{}\n".format(time.time(), "new game"))
    print("gamelog : new game")

    for enigma in enigmas:
        dup = deepcopy(enigma)
        log_file.write("{}\t{}\n".format(time.time(), "new enigma"))
        print("gamelog : new enigma")

        device.set_enigma(dup)
        while not device.solve_enigma():
            # On reboot
            if device.reboot:
                device.reboot = False
                log_file.write("{}\t{}\n".format(time.time(), "reboot"))
                print("gamelog : new enigma")
                return
            log_file.flush()
            # On error set colors
            device.send_state()
            time.sleep(2)

            # Reinit the current enigma
            dup = deepcopy(enigma)
            device.set_enigma(dup)
            time.sleep(.01)
    device.send_win_animation()
    time.sleep(30)

