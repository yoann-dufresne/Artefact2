"""Real hardware interface.

These is used to control the physical hardware.
"""

import os
import time
import logging
import errno

from game.hardware.abstract import AbstractDevice


from game.state import State, ARDUINOS_CONNECTED_TO_PANELS, ARDUINO_LED_STRIPS_ID, REBOOT_ARDUINO
# from network import MasterNetwork
from game.ArtefactServer import ArtefactServer

from game.hardware.button import Button


logger = logging.getLogger('root')
THRESHOLD_DOUBLE_CLICK = 1  # seconds


def symlink_force(target, link_name):
    try:
        os.symlink(target, link_name)
    except OSError as e:
        if e.errno == errno.EEXIST:
            os.remove(link_name)
            os.symlink(target, link_name)
        else:
            raise e


def change_difficulty(fname):
    """Change the game difficulty.

    We change the symlink to the "data" file and restart the service.
    """
    file_dir = os.path.dirname(os.path.realpath(__file__))
    data_dir = os.path.normpath(os.path.join(file_dir, "..", "data"))
    src = os.path.join(data_dir, fname)
    dst = os.path.join(data_dir, "culture.txt")
    symlink_force(src, dst)


class Device(AbstractDevice):
    """Interface to a device."""

    def __init__(self):
        """Initialisation."""
        super(Device, self).__init__()

        self.server = ArtefactServer()
        self.server.start()
        self.last_clicks = {}

    def set_enigma(self, enigma):
        self.enigma = enigma
        self.server.enigma = enigma
        self.send_state()

    def send_state(self):
        """Send the state to the hardware."""
        self.state = self.enigma.get_state()
        messages = self.notify_slaves()
        for message in messages:
            # print(message)
            self.server.send_game_msg(*message)

    def wait_for_event(self):
        time.sleep(0.1)
        while self.network.arduino_messages:
            arduino_id, msg = self.network.arduino_messages.popleft()

            try:
                panel_id = ARDUINOS_CONNECTED_TO_PANELS.index(int(arduino_id))
            except Exception as e:
                logger.exception("Got message {} from unknown arduino: {}".format(msg, arduino_id))
                continue

            if not msg.startswith("button"):
                continue

            _, button_id, status = msg.strip().split("-")
            current_ts = time.time()
            last_click_ts = self.last_clicks.get((panel_id, button_id, status), 0)
            if (current_ts - last_click_ts) < THRESHOLD_DOUBLE_CLICK:
                continue
            self.last_clicks[(panel_id, button_id, status)] = current_ts

            color = self.state.buttons[panel_id][int(button_id)].state
            button_changed = Button(panel_id, button_id, status, color)
            button_exists_in_enigma = self.enigma.button_triggered(button_changed)
            self.log_game("button pushed", button_changed)
            if button_exists_in_enigma and status == Button.BUTTON_DOWN:
                self.send_button_click()

    def notify_slaves(self):
        """Put the current state to the slaves in the message_to_slaves inbox."""
        messages_to_slaves = []
        messages_to_slaves.extend(self.build_led_strip_strings())
        messages_to_slaves.extend(self.build_led_buttons_strings())
        return messages_to_slaves

    def send_fade_out(self):
        message = (8, bytes([ord('L'), 8, 1])) # Fade out
        self.server.send_game_msg(*message)

    def send_button_click(self):
        message = ("sound", "validation")
        self.network.messages_to_slaves.append(message)

    def send_win_animation(self):
        message = (str(ARDUINO_LED_STRIPS_ID), "2")
        self.network.messages_to_slaves.append(message)


    def send_colors(self):
        colors = self.get_colors()

        msgs = []
        for i in range(0, 9):
            msgs.append((i, colors))

        return msgs

    def build_led_strip_strings(self):
        """Build the messages to set the led strips colors."""
        commande = ord("L")
        animation = 0 # No animation

        tmp = []
        for index, colors in enumerate(self.state.led_stripes):
            colors_formatted = [self.state.color_to_index(c) for c in colors]
            tmp.append((8, bytes([commande, index, animation] + colors_formatted)))
        return tmp

    def build_led_buttons_strings(self):
        """Build the messages to set the buttons colors."""
        tmp = []
        commande = ord("L") # LEDs
        for panel_id in range(8):
            # 8 leds
            colors = [butt.state for butt in self.state.normal_button_states()[panel_id]]
            colors_formatted = [self.state.color_to_index(c) for c in colors]
            # Central button
            if self.state.swag_button_states()[panel_id].state == "blanc":
                colors_formatted.append(1)
            else:
                colors_formatted.append(0)
            # Byte format
            tmp.append((panel_id, bytes([commande] + colors_formatted)))
        return tmp



