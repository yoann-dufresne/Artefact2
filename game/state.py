"""Modelisation for the game."""
from game.hardware.button import Button

COLORS = {
    "noir": 0,
    "rouge": 1,
    "vert": 2,
    "bleu": 3,
    "jaune": 4,
    "mauve": 5,
    "turquoise": 6,
    "orange": 7,
    "blanc": 8,
}


COLORS_ENCODING = {
    "noir": 'n',
    "rouge": 'r',
    "vert": 'v',
    "bleu": 'l',
    "jaune": 'j',
    "mauve": 'm',
    "turquoise": 't',
    "orange": 'o',
    "blanc": 'b'
}



class State():
    """Class to store the game state.

    The state is modified by the different enigmas.

    The state is also is charge to build the messages to the slaves.
    this is why it needs the "message_to_slaves" box.
    """

    def __init__(self):
        """Empty state."""
        self.init_led_strips()
        self.init_buttons()

        self.swag_button_id = 8

    def init_buttons(self):
        self.buttons = []
        for panel_id in range(8):
            tmp = []
            for button_id in range(9):
                tmp.append(Button(panel_id, button_id))
            self.buttons.append(tmp)

    def init_led_strips(self):
        self.led_stripes = [
            ["vert" for i in range(32)],  
            ["vert" for i in range(32)],  
            ["vert" for i in range(32)],
            ["vert" for i in range(32)],
            ["vert" for i in range(32)],
            ["vert" for i in range(32)],
            ["vert" for i in range(32)],
            ["vert" for i in range(32)],  
        ]

    def swag_button_states(self):
        return [panel[self.swag_button_id] for panel in self.buttons]

    def normal_button_states(self):
        res = []
        for panel in self.buttons:
            tmp = []
            for button_id, button in enumerate(panel):
                if button_id != self.swag_button_id:
                    tmp.append(button)
            res.append(tmp)
        return res
        # return [panel[:-1] for panel in self.buttons]

    def __repr__(self):
        res = ""
        res += "Led strips: \n"
        for strip in self.led_stripes:
            res += "  * ".join(strip) + "\n"

        res += "\n\n"
        res += "Swag Buttons : \n"
        for button in self.swag_button_states():
            res += "{} - ".format(button.state)

        return res

    def set_one_led_in_strip(self, strip_id, led_id, color):
        """Cache the color of a given led in a given panel to a given color."""
        self.led_stripes[strip_id][led_id] = color

    def set_all_leds_in_strip(self, strip_id, color):
        self.led_stripes[strip_id] = [color for _ in self.led_stripes[strip_id]]

    def set_swag_button(self, panel_id, value):
        """Cache the swag button led state to a given value."""
        self.buttons[panel_id][self.SWAG_BUTTON_ID].state = value

    def set_all_led_strips(self, color):
        """Set all led strips to a given color."""
        for strip_id in range(8):
            self.set_all_leds_in_strip(strip_id, color)

    def set_all_swag_buttons(self, status):
        """Set all swag buttons to the same status (True or False)."""
        for panel_id in range(8):
            self.set_swag_button(panel_id, status)

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        return self.led_stripes == other.led_stripes


    def octopus_messages(self):
        global COLORS_ENCODING
        msgs = []

        for idx, strip in enumerate(self.led_stripes):
            msgs.append(f'octopus {idx} {"".join([COLORS_ENCODING[c] for c in strip])}')

        return msgs

    def panel_messages(self):
        global COLORS_ENCODING
        msgs = []

        for idx, buttons in enumerate(self.buttons):
            msgs.append(f'panel{idx} {"".join([COLORS_ENCODING[btn.state] for btn in buttons])}')

        return msgs




class WrongState(State):
    """Class to store the game state.

    The state is modified by the different enigmas.

    The state is also is charge to build the messages to the slaves.
    this is why it needs the "message_to_slaves" box.
    """

    def __init__(self):
        """Empty state."""
        self.init_led_strips()
        self.init_buttons()

    def init_buttons(self):
        self.buttons = []
        for panel_id in range(8):
            tmp = []
            for button_id in range(9):
                tmp.append(Button(panel_id, button_id))
            self.buttons.append(tmp)

    def init_led_strips(self):
        self.led_stripes = [
            ["rouge" for i in range(32)],  
            ["rouge" for i in range(32)],  
            ["rouge" for i in range(32)],
            ["rouge" for i in range(32)],
            ["rouge" for i in range(32)],
            ["rouge" for i in range(32)],
            ["rouge" for i in range(32)],
            ["rouge" for i in range(32)], 
        ]




