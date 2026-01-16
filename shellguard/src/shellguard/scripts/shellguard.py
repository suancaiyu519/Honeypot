#!/usr/bin/env python

"""
ShellGuard service management script.

This script provides functionality to start, stop, restart, and check the status
of the ShellGuard honeypot service using the Twisted application framework.
"""

from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
import time
from pathlib import Path
from typing import NoReturn


def find_shellguard_directory() -> Path:
    """Determine the ShellGuard directory based on the script location."""
    script_path = Path(__file__).resolve()
    # Go up from scripts/shellguard.py to src/shellguard to root
    return script_path.parent.parent.parent.parent


def get_pid_file() -> Path:
    """Get the path to the PID file."""
    shellguard_dir = find_shellguard_directory()
    return shellguard_dir / "var" / "run" / "shellguard.pid"


def read_pid() -> int | None:
    """Read the PID from the PID file, return None if not found or invalid."""
    pid_file = get_pid_file()
    try:
        with pid_file.open() as f:
            return int(f.read().strip())
    except (FileNotFoundError, ValueError):
        return None


def is_process_running(pid: int) -> bool:
    """Check if a process with the given PID is running."""
    try:
        os.kill(pid, 0)
    except (OSError, ProcessLookupError):
        return False
    return True


def remove_stale_pidfile() -> None:
    """Remove the PID file if it exists."""
    pid_file = get_pid_file()
    if pid_file.exists():
        pid_file.unlink()
        print(f"Removed stale PID file {pid_file}")


def shellguard_status() -> None:
    """Print the current status of ShellGuard."""
    pid = read_pid()
    if pid is None:
        print("shellguard is not running.")
        return

    if is_process_running(pid):
        print(f"shellguard is running (PID: {pid}).")
    else:
        print(f"shellguard is not running (PID: {pid}).")
        remove_stale_pidfile()


def setup_environment() -> None:
    """Set up the environment for running ShellGuard."""
    shellguard_dir = find_shellguard_directory()
    os.chdir(shellguard_dir)


def first_time_use() -> None:
    """Display first-time use message."""
    shellguard_dir = find_shellguard_directory()
    log_file = shellguard_dir / "var" / "log" / "shellguard" / "shellguard.log"

    if not log_file.exists():
        print()
        print("Join the ShellGuard community at: https://www.shellguard.org/slack/")
        print()


def python_version_warning() -> None:
    """Display Python version warnings if needed."""
    version_info = sys.version_info

    if version_info < (3, 10):
        print()
        print("DEPRECATION: Python<3.10 is no longer supported by ShellGuard.")
        print()


def check_root() -> None:
    """Check if running as root and exit if so."""
    if os.name == "posix" and os.getuid() == 0:
        print("ERROR: You must not run shellguard as root!")
        sys.exit(1)


def shellguard_start(args: list[str]) -> NoReturn:
    """Start the ShellGuard service."""
    setup_environment()
    first_time_use()
    python_version_warning()

    # Check if already running
    pid = read_pid()
    if pid is not None and is_process_running(pid):
        print(f"shellguard is already running (PID: {pid}).")
        sys.exit(1)

    # Remove stale PID file if it exists
    if pid is not None:
        remove_stale_pidfile()

    # Build twistd arguments
    twisted_args = ["twistd", "--umask=0022"]

    # Add PID file unless running in foreground
    stdout_mode = os.environ.get("SHELLGUARD_STDOUT", "").lower() == "yes"
    if not stdout_mode:
        pid_file = get_pid_file()
        # Ensure PID file directory exists
        pid_file.parent.mkdir(parents=True, exist_ok=True)
        twisted_args.extend(["--pidfile", str(pid_file)])
        twisted_args.extend(["--logger", "shellguard.python.logfile.logger"])
    else:
        twisted_args.extend(["-n", "-l", "-"])

    # Add any additional arguments passed to the script
    twisted_args.extend(args)

    # Add the shellguard plugin
    twisted_args.append("shellguard")

    print(f"Starting shellguard: [{' '.join(twisted_args)}]...")

    # Check for authbind
    authfile = Path("/etc/authbind/byport/22")
    authbind_enabled = (
        os.environ.get("AUTHBIND_ENABLED", "").lower() != "no"
        and authfile.exists()
        and os.access(authfile, os.X_OK)
        and subprocess.run(["which", "authbind"], capture_output=True).returncode == 0
    )

    if authbind_enabled:
        twisted_args.insert(0, "--deep")
        twisted_args.insert(0, "authbind")

    # Execute twistd
    os.execvp(twisted_args[0], twisted_args)


def shellguard_stop() -> None:
    """Stop the ShellGuard service."""
    pid = read_pid()
    if pid is None:
        print("shellguard is not running.")
        return

    if not is_process_running(pid):
        print("shellguard is not running.")
        remove_stale_pidfile()
        return

    print("Stopping shellguard...")
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        print("shellguard is not running.")
        remove_stale_pidfile()


def shellguard_force_stop() -> None:
    """Force stop the ShellGuard service."""
    pid = read_pid()
    if pid is None:
        print("shellguard is not running.")
        return

    if not is_process_running(pid):
        print("shellguard is not running.")
        remove_stale_pidfile()
        return

    print("Stopping shellguard...", end="", flush=True)
    try:
        os.kill(pid, signal.SIGTERM)

        # Wait up to 60 seconds for graceful shutdown
        for _ in range(60):
            time.sleep(1)
            print(".", end="", flush=True)
            if not is_process_running(pid):
                print("terminated.")
                return

        # Force kill if still running
        os.kill(pid, signal.SIGKILL)
        print("killed.")
    except ProcessLookupError:
        print("\nshellguard is not running.")
        remove_stale_pidfile()


def shellguard_restart(args: list[str]) -> NoReturn:
    """Restart the ShellGuard service."""
    shellguard_stop()
    time.sleep(2)  # Brief pause to ensure clean shutdown
    shellguard_start(args)


def shellguard_shell() -> NoReturn:
    """Launch a shell (mainly for Docker use)."""
    shell = os.environ.get("SHELL", "/bin/bash")
    os.execvp(shell, [shell])


def main() -> NoReturn:
    """Main entry point for the shellguard management script."""
    check_root()
    parser = argparse.ArgumentParser(description="ShellGuard honeypot service manager")
    parser.add_argument(
        "command",
        choices=[
            "start",
            "stop",
            "force-stop",
            "restart",
            "status",
            "shell",
            "bash",
            "sh",
        ],
        help="Command to execute",
    )
    parser.add_argument(
        "args", nargs="*", help="Additional arguments to pass to twistd"
    )

    parsed_args = parser.parse_args()

    if parsed_args.command == "start":
        shellguard_start(parsed_args.args)
    elif parsed_args.command == "stop":
        shellguard_stop()
        sys.exit(0)
    elif parsed_args.command == "force-stop":
        shellguard_force_stop()
        sys.exit(0)
    elif parsed_args.command == "restart":
        shellguard_restart(parsed_args.args)
    elif parsed_args.command == "status":
        shellguard_status()
        sys.exit(0)
    elif parsed_args.command in ("shell", "bash", "sh"):
        shellguard_shell()
    else:
        parser.print_help()
        sys.exit(1)


def run() -> NoReturn:
    """Entry point function for setuptools console script."""
    main()


if __name__ == "__main__":
    main()
