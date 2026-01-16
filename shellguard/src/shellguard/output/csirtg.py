from __future__ import annotations
import os
import sys
from datetime import datetime

from twisted.python import log

import shellguard.core.output
from shellguard.core.config import ShellGuardConfig

token = ShellGuardConfig.get("output_csirtg", "token", fallback="a1b2c3d4")
if token == "a1b2c3d4":
    log.msg("output_csirtg: token not found in configuration file")
    sys.exit(1)

os.environ["CSIRTG_TOKEN"] = token
import csirtgsdk  # noqa: E402


class Output(shellguard.core.output.Output):
    """
    CSIRTG output
    """

    def start(self):
        """
        Start the output module.
        Note that csirtsdk is imported here because it reads CSIRTG_TOKEN on import
        ShellGuard sets this environment variable.
        """
        self.user = ShellGuardConfig.get("output_csirtg", "username")
        self.feed = ShellGuardConfig.get("output_csirtg", "feed")
        self.debug = ShellGuardConfig.getboolean("output_csirtg", "debug", fallback=False)
        self.description = ShellGuardConfig.get("output_csirtg", "description")

        self.context = {}
        # self.client = csirtgsdk.client.Client()

    def stop(self):
        pass

    def write(self, event):
        """
        Only pass on connection events
        """
        if event["eventid"] == "shellguard.session.connect":
            self.submitIp(event)

    def submitIp(self, e):
        peerIP = e["src_ip"]
        ts = e["timestamp"]
        system = e.get("system", None)

        if system not in [
            "shellguard.ssh.factory.ShellGuardSSHFactory",
            "shellguard.telnet.transport.HoneyPotTelnetFactory",
        ]:
            return

        today = str(datetime.now().date())

        if not self.context.get(today):
            self.context = {}
            self.context[today] = set()

        key = ",".join([peerIP, system])

        if key in self.context[today]:
            return

        self.context[today].add(key)

        tags = "scanner,ssh"
        port = 22
        if e["system"] == "shellguard.telnet.transport.HoneyPotTelnetFactory":
            tags = "scanner,telnet"
            port = 23

        i = {
            "user": self.user,
            "feed": self.feed,
            "indicator": peerIP,
            "portlist": port,
            "protocol": "tcp",
            "tags": tags,
            "firsttime": ts,
            "lasttime": ts,
            "description": self.description,
        }

        if self.debug is True:
            log.msg(f"output_csirtg: Submitting {i!r} to CSIRTG")

        ind = csirtgsdk.indicator.Indicator(i).submit()

        if self.debug is True:
            log.msg(f"output_csirtg: Submitted {ind!r} to CSIRTG")

        log.msg("output_csirtg: submitted to csirtg at {} ".format(ind["location"]))
