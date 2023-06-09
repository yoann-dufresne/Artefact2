from game.hardware.button import Button
from game.state import State

SWAG_BUTTON_ID = 8


class Enigma:
    def __init__(self):
        self.sub_enigmas = []
        self.buttons_mapping = {}
        self.on_error = False

    def add_sub_enigma(self, sub_enigma):
        self.sub_enigmas.append(sub_enigma)
        for button in sub_enigma.buttons_of_interest():
            self.buttons_mapping[button] = sub_enigma

    def is_solved(self):
        return all((se.is_solved() for se in self.sub_enigmas))

    def vector_solved(self):
        return [se.is_solved() for se in self.sub_enigmas]

    def button_triggered(self, button):
        if button in self.buttons_mapping:
            enigma = self.buttons_mapping[button]
            self.on_error = not enigma.button_trigger(button)
            return True
        return False

    def set_wrong(self):
        self.buttons_mapping = []

    def get_state(self):
        st = State()

        # état initial de tous les bandeaux de leds
        st.init_led_strips()

        # état initial de tous les boutons de tous les panneaux
        st.init_buttons()

        # Valable uniquement car les status ne s'appliquent qu'à un bandeau à chaque fois
        if self.on_error:
            st.set_all_led_strips("rouge")
        elif not self.is_solved():
            for sub in self.sub_enigmas:
                # Récupère le status du bandeau pour la sous énigme
                sub_status = sub.get_led_status()

                # Met à jour le bandeau dans l'objet status
                for i in range(32):
                    if not sub_status[i] is None:
                        st.led_stripes[sub.interest_id][i] = sub_status[i]

                for button in sub.buttons_of_interest():
                    st.buttons[button.panel][button.button].state = button.state

        return st


class SubEnigma:
    def __init__(self):
        pass

    def is_solved(self):
        raise NotImplementedError

    def get_led_status(self):
        """Renvoie un iterable de 32 couleurs, pouvant être None si l'on ne considère pas la led."""
        raise NotImplementedError

    def button_trigger(self, button):
        """
        button : un dictionnaire avec panel_id, button_id, status
        """
        raise NotImplementedError

    def buttons_of_interest(self):
        raise NotImplementedError

    # todo : init buttons
    def __repr__(self):
        return "SubEnigma ({}) : {}".format(self.name, self.led_strip_status)


class SwagEnigma(SubEnigma):
    def __init__(self, message):
        """
        interest_id : the id of the panel / strip to listen to (the red strip, and the ID of the panel to press)
        led_strip_status: iterable of 8 booleans with the leds to consider
        """
        self.name = "Swag Enigma"
        _, interest_id, led_strip_status = message.strip().split()
        led_strip_status = [c == "x" for c in led_strip_status]

        self.interest_id = int(interest_id)
        self.panel_id = interest_id
        self.led_strip_status = led_strip_status
        self.solved = False

        self.button = Button(self.panel_id, SWAG_BUTTON_ID, Button.BUTTON_UP, "blanc")

    def is_solved(self):
        return self.solved

    def get_led_status(self):
        status = []
        for led_status in self.led_strip_status:
            color = "rouge" if (not self.solved) and led_status else None
            for _ in range(4):
                status.append(color)
        return status

    def button_trigger(self, button):
        if button.status == Button.BUTTON_DOWN:
            if self.solved:
                self.solved = False
            else:
                self.solved = True
            return True
        return True

    def buttons_of_interest(self):
        return [self.button]


class ButtonEnigma(SubEnigma):

    def __init__(self, message):
        self.name = "Button"
        _, panel_id, button_statuses = message.strip().split()
        self.panel_id = panel_id
        self.buttons = []

        self.colors = {
            "r" : "rouge",
            "l" : "bleu",
            "j" : "jaune",
            "m" : "mauve",
            "o" : "orange",
            "v" : "vert",
            "t" : "turquoise",
            "b" : "blanc",
            "n" : "noir"
        }

        for index, char in enumerate(button_statuses[:-1]):
            if char != ".":
                self.buttons.append(Button(self.panel_id, index, Button.BUTTON_UP, self.colors[char]))

        if button_statuses[-1] == "T":
            self.buttons.append(Button(self.panel_id, SWAG_BUTTON_ID, Button.BUTTON_UP, "blanc"))

    def is_solved(self):
        return True

    def get_led_status(self):
        return [None] * 32

    def button_trigger(self, button):
        print(button)
        if button.status == Button.BUTTON_DOWN:
            return False
        else:
            return True

    def buttons_of_interest(self):
        return self.buttons


class LittleEnigma(SubEnigma):

    def __init__(self, message):
        self.name = "Little"
        _, panel_id, led_status = message.strip().split()
        self.interest_id = int(panel_id)
        self.init_led_status = [c == "x" for c in led_status]
        self.solved = [not i for i in self.init_led_status]

        self.buttons = []
        for index, value in enumerate(self.solved):
            if not value:
                self.buttons.append(Button(index, self.interest_id, Button.BUTTON_UP, "bleu"))

    def is_solved(self):
        return all(self.solved)

    def get_led_status(self):
        status = []
        for solved in self.solved:
            color = None if solved else "bleu"
            for i in range(4):
                status.append(color)

        return status

    def button_trigger(self, button):
        if button.status == Button.BUTTON_DOWN:
            if not self.solved[button.panel]:
                self.solved[button.panel] = True
            else:
                self.solved[button.panel] = False
        return True

    def buttons_of_interest(self):
        return self.buttons


class DarkEnigma(SubEnigma):

    def __init__(self, message):
        self.name = "Dark"
        _, panel_id = message.strip().split()
        self.interest_id = int(panel_id)
        self.mask = True

    def is_solved(self):
        return True

    def get_led_status(self):
        if self.mask:
            return ["noir"] * 32
        else:
            return [None] * 32

    def button_trigger(self, button):
        if button.status == Button.BUTTON_DOWN:
            self.mask = False
        else:
            self.mask = True

        return True

    def buttons_of_interest(self):
        return [Button((self.interest_id + 4) % 8, SWAG_BUTTON_ID, Button.BUTTON_UP, "blanc")]


class SwagLittleEnigma(SubEnigma):

    def __init__(self, message):
        self.name = "SwagLittle"
        _, panel_id, led_status = message.strip().split()
        self.init_led_status = [c == "x" for c in led_status]
        self.status = ["jaune" if value else None for value in self.init_led_status]
        self.interest_id = int(panel_id)

    def is_solved(self):
        solved = [value == None for value in self.status]
        return all(solved)

    def get_led_status(self):
        leds = []
        for status in self.status:
            for i in range(4):
                leds.append(status)

        return leds

    def button_trigger(self, button):
        if button.status == Button.BUTTON_DOWN:
            # Swag button
            if button.button == SWAG_BUTTON_ID:
                completed = [value != "jaune" for value in self.status]
                if all(completed):
                    self.status = [None] * 8
                    return True
                else:
                    return False
            # Little button
            else:
                if self.status[button.panel] == "jaune":
                    self.status[button.panel] = "mauve"
                else:
                    self.status[button.panel] = "jaune"
                return True
        else:
            return True

    def buttons_of_interest(self):
        # SWAG button
        self.buttons = [Button(self.interest_id, SWAG_BUTTON_ID, Button.BUTTON_UP, "blanc")]
        # little buttons
        for index, value in enumerate(self.status):
            if value != None:
                self.buttons.append(Button(index, self.interest_id, Button.BUTTON_UP, value))

        return self.buttons



class SequenceEnigma(SubEnigma):

    def __init__(self, message):
        self.name = "Sequence"
        _, self.interest_id, led_status = message.strip().split()
        self.interest_id = int(self.interest_id)

        colors_macro = {
            "r" : "rouge",
            "l" : "bleu",
            "j" : "jaune",
            "m" : "mauve",
            "o" : "orange",
            "t" : "turquoise",
            "b" : "blanc",
            "n" : "noir"
        }

        self.stack = []
        self.colors = [None] * 16
        self.solved = [True] * 16
        for i in range(16):
            # No color
            if led_status[i] == ".":
                continue

            # color
            self.colors[i] = colors_macro[led_status[i]]
            self.solved[i] = False
            self.stack.append(self.colors[i])

        self.buttons = []
        idx = 0
        for val in colors_macro.values():
            self.buttons.append(Button(self.interest_id, idx, Button.BUTTON_UP, val))
            idx += 1


    def is_solved(self):
        return all(self.solved)

    def get_led_status(self):
        leds = []
        for idx, color in enumerate(self.colors):
            for _ in range(2):
                if not self.solved[idx]:
                    leds.append(color)
                else:
                    leds.append(None)

        return leds

    def button_trigger(self, button):
        if button.status == Button.BUTTON_DOWN:
            print(self.stack)
            if not self.stack:
                return False

            color = self.stack.pop(0)
            if self.buttons[button.button].state == color:
                for idx, val in enumerate(self.solved):
                    if not val:
                        self.solved[idx] = True
                        return True
            return False
        else:
            return True

    def buttons_of_interest(self):
        return self.buttons


