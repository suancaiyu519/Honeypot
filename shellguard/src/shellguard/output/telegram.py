# Simple Telegram Bot logger

import treq
from twisted.python import log
import shellguard.core.output
from shellguard.core.config import ShellGuardConfig


class Output(shellguard.core.output.Output):
    """
    telegram output
    """

    def start(self):
        self.bot_token = ShellGuardConfig.get("output_telegram", "bot_token")
        self.chat_id = ShellGuardConfig.get("output_telegram", "chat_id")

    def stop(self):
        pass

    def write(self, event):
        for i in list(event.keys()):
            # remove twisted 15 legacy keys
            if i.startswith("log_"):
                del event[i]

        # Prepare logon type
        # if "HoneyPotSSHTransport" in (event["system"].split(","))[0]:
        #     logon_type = "SSH"
        # elif "ShellGuardTelnetTransport" in (event["system"].split(","))[0]:
        #     logon_type = "Telnet"
        # else:
        #     logon_type = ""

        # Prepare base message
        msgtxt = "<strong>[ShellGuard " + event["sensor"] + "]</strong>"
        msgtxt += "\nEvent: " + event["eventid"]
        # msgtxt += "\nLogon type: " + logon_type
        msgtxt += "\nSource: <code>" + event["src_ip"] + "</code>"
        msgtxt += "\nSession: <code>" + event["session"] + "</code>"

        if event["eventid"] == "shellguard.login.success":
            msgtxt += "\nUsername: <code>" + event["username"] + "</code>"
            msgtxt += "\nPassword: <code>" + event["password"] + "</code>"
            self.send_message(msgtxt)
        elif event["eventid"] in ["shellguard.command.failed", "shellguard.command.input"]:
            msgtxt += "\nCommand: <pre>" + event["input"] + "</pre>"
            self.send_message(msgtxt)
        elif event["eventid"] == "shellguard.session.file_download":
            msgtxt += "\nUrl: " + event.get("url", "")
            self.send_message(msgtxt)

    def send_message(self, message):
        log.msg("Telegram plugin will try to call TelegramBot")
        try:
            treq.get(
                "https://api.telegram.org/bot" + self.bot_token + "/sendMessage",
                params=[
                    ("chat_id", str(self.chat_id)),
                    ("parse_mode", "HTML"),
                    ("text", message),
                ],
            )
        except Exception:
            log.msg("Telegram plugin request error")
