frozen importlib._bootstrap>:488: RuntimeWarning: Your system is avx2 capable but pygame was not built with support for it. The performance of some of your blits could be adversely affected. Consider enabling compile time detection with environment variables like PYGAME_DETECT_AVX2=1 if you are compiling without cross compilation.
pygame 2.5.2 (SDL 2.30.0, Python 3.12.3)
Hello from the pygame community. https://www.pygame.org/contribute.html
Artefect version 2
Connecting to gateway at 192.168.4.1:8080...
Sending registration message: register game server D4:54:8B:0A:7B:3E
gamelog : new game
gamelog : new enigma
Received response: port 8082
Connecting to dedicated port 8082...
Exception in thread Thread-1:
Traceback (most recent call last):
  File "/usr/lib/python3.12/threading.py", line 1073, in _bootstrap_inner
    self.run()
  File "/home/yoann/Projects/Games/Artefact2/game/hardware/Gateway.py", line 34, in run
    sock = self.connect()
           ^^^^^^^^^^^^^^
  File "/home/yoann/Projects/Games/Artefact2/game/hardware/Gateway.py", line 95, in connect
    sock = self.start_dedicated_connection(port)
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/home/yoann/Projects/Games/Artefact2/game/hardware/Gateway.py", line 133, in start_dedicated_connection
    sock.connect(("192.168.4.1", port))
ConnectionRefusedError: [Errno 111] Connection refused
