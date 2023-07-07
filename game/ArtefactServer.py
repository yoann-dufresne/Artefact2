from network.Server import ESPServer
from time import time

from game.hardware.button import Button


class ArtefactServer(ESPServer):

    def __init__(self):
        super().__init__()
        self.panels = [None]*9
        self.not_assigned = []

        self.inbox = []

        self.hardware_macs = {
            bytes([100, 183, 8, 64, 177, 212, 0, 0]): 0,
            bytes([160, 183, 101, 108, 140, 20, 0, 0]): 1,
            bytes([124, 135, 206, 39, 176, 84, 0, 0]): 2,
            bytes([160, 183, 101, 78, 79, 148, 0, 0]): 3,
            bytes([64, 34, 216, 234, 202, 208, 0, 0]): 4,
            bytes([8, 182, 31, 40, 214, 160, 0, 0]): 5,
            bytes([8, 182, 31, 41, 214, 244, 0, 0]): 6,
            bytes([160, 183, 101, 77, 138, 28, 0, 0]): 7,
            bytes([192, 73, 239, 205, 188, 24, 0, 0]): 8
        }

        self.sending_boxes = [[] for _ in range(9)]

        that = self
        def callback(client):
            return that.connection_callback(client)
        self.handlers.append(callback)


    def connection_callback(self, esp_client):
        that = self
        def callback(client, msg):
            return that.msg_handler(client, msg)
        esp_client.register_message_handler(callback)

        self.not_assigned.append(esp_client)

    def correct_msg(self, panel_id, msg):
        transformed = list(msg)

        if panel_id % 2 == 0:
            for btn_idx in range(0, 8):
                transformed[1 + ((btn_idx + 8 - panel_id) % 8)] = msg[1 + btn_idx]
        else:
            for btn_idx in range(0, 8):
                transformed[1 + ((btn_idx + 8 - (panel_id - 3)) % 8)] = msg[1 + btn_idx]

        return bytes(transformed)

    def send_game_msg(self, panel_id, msg):
        if panel_id < 0 or panel_id > 8:
            return

        if self.panels[panel_id] is not None:
            if panel_id < 8 and msg[0] == ord('L'):
                msg = self.correct_msg(panel_id, msg)

            print(f"sending to {panel_id}: {msg}")
            self.panels[panel_id].send_msg(msg)
        else:
            self.sending_boxes[panel_id].append(msg)


    def msg_handler(self, esp_client, msg):
        print(f"Message from {esp_client.mac}: ", repr(msg))

        # Only panels correctly set
        if esp_client.mac not in self.hardware_macs:
            return
        panel_id = self.hardware_macs[esp_client.mac]


        msg = list(msg)
        # on button message
        if (msg[0] == ord('B')):
            # parse msg
            btn_idx = msg[1]
            pushed = msg[2] == 1

            if (btn_idx == 8):
                self.inbox.append(Button(panel_id, btn_idx, Button.BUTTON_DOWN if pushed else Button.BUTTON_UP))
            elif (panel_id % 2 == 0):
                self.inbox.append(Button(panel_id, (btn_idx + panel_id) % 8, Button.BUTTON_DOWN if pushed else Button.BUTTON_UP))
            else:
                self.inbox.append(Button(panel_id, (8 + btn_idx + panel_id - 3) % 8, Button.BUTTON_DOWN if pushed else Button.BUTTON_UP))


    def get_colors(self):
        return bytes([ord('C'),
            0, 0, 0,
            40, 1, 1,
            1, 35, 2,
            2, 2, 50,
            30, 28, 0,
            30, 0, 40,
            0, 35, 25,
            40, 15, 0,
            20, 20, 20
        ])


    def esp_connected(self, client):
        if client.mac in self.hardware_macs:
            idx = self.hardware_macs[client.mac]

            if 0 <= idx <= 8:
                # Register new panel
                self.panels[idx] = client
                # send messages
                mails = [self.get_colors()] + self.sending_boxes[idx]
                self.sending_boxes[idx] = []
                for msg in mails:
                    self.send_game_msg(idx, msg)


