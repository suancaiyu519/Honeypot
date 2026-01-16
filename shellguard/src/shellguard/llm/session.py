# Copyright (c) 2024 Michel Oosterhof <michel@oosterhof.net>
# See the COPYRIGHT file for more information

from __future__ import annotations

from zope.interface import implementer

from twisted.conch.interfaces import ISession
from twisted.conch.ssh import session
from twisted.python import log

from shellguard.insults import insults

from shellguard.llm import protocol


@implementer(ISession)
class SSHSessionForShellGuardUser:
    def __init__(self, avatar, reactor=None):
        """
        Construct an C{SSHSessionForShellGuardUser}.

        @param avatar: The L{ShellGuardUser} for whom this is an SSH session.
        @param reactor: An L{IReactorProcess} used to handle shell and exec
            requests. Uses the default reactor if None.
        """
        self.protocol = None
        self.avatar = avatar
        self.server = avatar.server
        self.username = avatar.username
        self.environ = {}

    def openShell(self, processprotocol):
        self.protocol = insults.LoggingServerProtocol(
            protocol.HoneyPotInteractiveProtocol, self
        )
        self.protocol.makeConnection(processprotocol)
        processprotocol.makeConnection(session.wrapProtocol(self.protocol))

    def getPty(self, terminal, windowSize, attrs):
        self.environ["TERM"] = terminal.decode("utf-8")
        log.msg(
            eventid="shellguard.client.size",
            width=windowSize[1],
            height=windowSize[0],
            format="Terminal Size: %(width)s %(height)s",
        )
        self.windowSize = windowSize

    def execCommand(self, processprotocol, cmd):
        self.protocol = insults.LoggingServerProtocol(
            protocol.HoneyPotExecProtocol, self, cmd
        )
        self.protocol.makeConnection(processprotocol)
        processprotocol.makeConnection(session.wrapProtocol(self.protocol))

    def closed(self) -> None:
        """
        this is reliably called on both logout and disconnect
        we notify the protocol here we lost the connection
        """
        if self.protocol:
            self.protocol.connectionLost("disconnected")
            self.protocol = None

    def eofReceived(self) -> None:
        if self.protocol:
            self.protocol.eofReceived()

    def windowChanged(self, windowSize):
        self.windowSize = windowSize
