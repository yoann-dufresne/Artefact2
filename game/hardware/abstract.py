import time
from game.state import State
from game.hardware.button import Button
# from playsound import 
from pygame import mixer


class AbstractDevice:
    """Interface to a device."""

    def __init__(self):
        """Initialisation."""
        self.state = State()
        print("device init")
        self.SWAG_BUTTON_ID = 8
        self.BUTTON_DOWN_CODE = "DOWN"
        self.enigma = None
        
        # Initialisation du sond
        mixer.pre_init(buffer=4096)
        mixer.init()
        mixer.music.load("data/atmosphere.mp3")
        self.button_sound = mixer.Sound("data/validation.mp3")
        mixer.music.play(loops=-1)

    def send_state(self):
        """Send the state to the hardware."""
        raise NotImplementedError

    def set_enigma(self, enigma):
        self.enigma = enigma
        self.send_state()

    def set_one_led_in_panel(self, panel_id, led_id, color):
        self.state.buttons[panel_id][led_id].state = color

    def log_game(self, message, parameters=None):
        # if parameters:
        #     self.log_file.write("{}\t{}\t{}\n".format(
        #         time.time(),
        #         message,
        #         parameters
        #     ))
        # else:
        #     self.log_file.write("{}\t{}\n".format(
        #         time.time(),
        #         message,
        #     ))
        # self.log_file.flush()
        pass

    def solve_enigma(self):
        """ Game loop for an enigma """
        state = self.enigma.get_state()
        vs = None
        while not self.enigma.is_solved():
            time.sleep(.01)

            if self.server.reboot:
                return

            # No new message
            if len(self.server.inbox) == 0:
                continue
            else:
                btn = self.server.inbox.pop(0)

                button_exists_in_enigma = self.enigma.button_triggered(btn)
                if button_exists_in_enigma and btn.status == Button.BUTTON_DOWN:
                    self.button_sound.play()

            if vs != self.enigma.vector_solved():
                vs = self.enigma.vector_solved()
                self.log_game("vector subenigmas solved", vs)

            if self.enigma.on_error:
                self.log_game("on error")
                return False

            if state != self.enigma.get_state():
                state = self.enigma.get_state()
                self.send_state()

        self.send_fade_out()
        time.sleep(5)

        return True

    def wait_for_event(self):
        """ waiting for game event """
        raise NotImplementedError

    def send_fade_out(self):
        pass

    def send_win_animation(self):
        pass
