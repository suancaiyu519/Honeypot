"""
ShellGuard Crashreport

This output plugin is not like the others.
It has its own emit() function and does not use shellguard eventid's
to avoid circular calls
"""

from __future__ import annotations


import json

import treq

from twisted.internet import defer
from twisted.logger._levels import LogLevel
from twisted.python import log

import shellguard.core.output
from shellguard._version import __version__
from shellguard.core.config import ShellGuardConfig

SHELLGUARD_USER_AGENT = f"ShellGuard Honeypot {__version__}".encode("ascii")
SHELLGUARD_URL = "https://api.shellguard.org/v1/crash"


class Output(shellguard.core.output.Output):
    """
    ShellGuard Crashreporter output
    """

    def start(self):
        """
        Start output plugin
        """
        self.apiKey = ShellGuardConfig.get("output_shellguard", "api_key", fallback=None)
        self.debug = ShellGuardConfig.getboolean("output_shellguard", "debug", fallback=False)

    def emit(self, event):
        """
        Note we override emit() here, unlike other plugins.
        """
        if event.get("log_level") == LogLevel.critical:
            self.crashreport(event)

    def stop(self):
        """
        Stop output plugin
        """
        pass

    def write(self, event):
        """
        events are done in emit() not in write()
        """
        pass

    @defer.inlineCallbacks
    def crashreport(self, entry):
        """
        Crash report
        """
        try:
            r = yield treq.post(
                SHELLGUARD_URL,
                json.dumps(
                    {"log_text": entry.get("log_text"), "system": entry.get("system")}
                ).encode("ascii"),
                headers={
                    b"Content-Type": [b"application/json"],
                    b"User-Agent": [SHELLGUARD_USER_AGENT],
                },
            )
            content = yield r.text()
            if self.debug:
                log.msg("crashreport: " + content)
        except Exception as e:
            log.msg("crashreporter failed" + repr(e))
