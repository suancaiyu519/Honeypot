from __future__ import annotations
import json
import socket

import shellguard.core.output
from shellguard.core.config import ShellGuardConfig


class Output(shellguard.core.output.Output):
    """
    socketlog output
    """

    def start(self):
        self.timeout = ShellGuardConfig.getint("output_socketlog", "timeout")
        addr = ShellGuardConfig.get("output_socketlog", "address")
        self.host = addr.split(":")[0]
        self.port = int(addr.split(":")[1])

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
        self.sock.connect((self.host, self.port))

    def stop(self):
        self.sock.close()

    def write(self, event):
        for i in list(event.keys()):
            # Remove twisted 15 legacy keys
            if i.startswith("log_"):
                del event[i]

        message = json.dumps(event) + "\n"

        try:
            self.sock.sendall(message.encode())
        except OSError as ex:
            if ex.errno == 32:  # Broken pipe
                self.start()
                self.sock.sendall(message.encode())
            else:
                raise
