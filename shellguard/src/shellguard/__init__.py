import sys
from twisted.python import log

try:
    import shellguard._version as shellguard_version

    __version__ = shellguard_version
except ModuleNotFoundError:
    log.err(
        "ShellGuard is not installed. Run `pip install -e .` to install ShellGuard into your virtual enviroment"
    )
    sys.exit(1)
