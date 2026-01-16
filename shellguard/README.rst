ShellGuard
######

What is ShellGuard
*****************************************

ShellGuard is a medium to high interaction SSH and Telnet honeypot
designed to log brute force attacks and the shell interaction
performed by the attacker. In medium interaction mode (shell) it
emulates a UNIX system in Python, in high interaction mode (proxy)
it functions as an SSH and telnet proxy to observe attacker behavior
to another system. In LLM mode, it uses large language models to
generate dynamic responses to attacker commands.

`ShellGuard <http://github.com/shellguard/shellguard/>`_ is maintained by Michel Oosterhof.

Documentation
****************************************

The Documentation can be found `here <https://docs.shellguard.org/en/latest/index.html>`_.

Slack
*****************************************

You can join the ShellGuard community at the following `Slack workspace <https://www.shellguard.org/slack/>`_.

Features
*****************************************

* Choose to run as an emulated shell (default):
   * Fake filesystem with the ability to add/remove files. A full fake filesystem resembling a Debian 5.0 installation is included
   * Possibility of adding fake file contents so the attacker can `cat` files such as `/etc/passwd`. Only minimal file contents are included
   * ShellGuard saves files downloaded with wget/curl or uploaded with SFTP and scp for later inspection

* Or proxy SSH and telnet to another system
   * Run as a pure telnet and ssh proxy with monitoring
   * Or let ShellGuard manage a pool of QEMU emulated servers to provide the systems to login to

* Or use an LLM backend (experimental):
   * Use large language models (e.g., OpenAI GPT) to dynamically generate realistic shell responses
   * Handles any command without predefined responses
   * Maintains conversation context for consistent sessions

For both settings:

* Session logs are stored in a `UML Compatible <http://user-mode-linux.sourceforge.net/>`_  format for easy replay with the `playlog` utility.
* SFTP and SCP support for file upload
* Support for SSH exec commands
* Logging of direct-tcp connection attempts (ssh proxying)
* Forward SMTP connections to SMTP Honeypot (e.g. `mailoney <https://github.com/awhitehatter/mailoney>`_)
* JSON logging for easy processing in log management solutions

Installation
*****************************************

There are currently three ways to install ShellGuard: `git clone`, `Docker` and `pip`.
`Docker` is the easiest to try and run, but to configure and modify you'll need a good understanding of containers and volumes.
`git clone` is recommended if you want to change the configuration of the honeypot.
`pip` mode is still under development.

Docker
*****************************************

`Docker images <https://hub.docker.com/repository/docker/shellguard/shellguard>`_ are available on Docker Hub.

* To get started quickly and give ShellGuard a try, run::

    $ docker run -p 2222:2222 shellguard/shellguard:latest
    $ ssh -p 2222 root@localhost

* To just make it locally, run::

    $ make docker-build

PyPI
*****************************************

`ShellGuard is available on PyPI <https://pypi.org/project/shellguard>`_, to install run::

    $ pip install shellguard
    $ twistd shellguard

When installed this way, it will behave differently from having a full directory download.

This is still in beta and may not work as expected, `git clone` or `docker` methods are preferred.

Requirements
*****************************************

Software required to run locally:

* Python 3.10+
* python-virtualenv

Files of interest:
*****************************************

* `etc/shellguard.cfg` - ShellGuard's configuration file.
* `etc/shellguard.cfg.dist <https://github.com/shellguard/shellguard/blob/main/etc/shellguard.cfg.dist>`_ - default settings, don't change this file
* `etc/userdb.txt` - credentials to access the honeypot
* `src/shellguard/data/fs.pickle` - fake filesystem, this only contains metadata (path, uid, gid, size)
* `honeyfs/ <https://github.com/shellguard/shellguard/tree/main/honeyfs>`_ - contents for the fake filesystem
* `honeyfs/etc/issue.net` - pre-login banner
* `honeyfs/etc/motd <https://github.com/shellguard/shellguard/blob/main/honeyfs/etc/issue>`_ - post-login banner
* `src/shellguard/data/txtcmds/` - output for simple fake commands
* `var/log/shellguard/shellguard.json` - audit output in JSON format
* `var/log/shellguard/shellguard.log` - log/debug output
* `var/lib/shellguard/tty/` - session logs, replayable with the `playlog` utility.
* `var/lib/shellguard/downloads/` - files transferred from the attacker to the honeypot are stored here

Commands
******************************************
* `shellguard` - start, stop and restart ShellGuard
* `fsctl` - modify the fake filesystem
* `createfs` - create your own fake filesystem
* `playlog` - utility to replay session logs
* `asciinema` - turn ShellGuard logs into asciinema files

Contributors
***************

Many people have contributed to ShellGuard over the years. Special thanks to:

* Upi Tamminen (desaster) for all his work developing Kippo on which ShellGuard was based
* Dave Germiquet (davegermiquet) for TFTP support, unit tests, new process handling
* Olivier Bilodeau (obilodeau) for Telnet support
* Ivan Korolev (fe7ch) for many improvements over the years.
* Florian Pelgrim (craneworks) for his work on code cleanup and Docker.
* Guilherme Borges (sgtpepperpt) for SSH and telnet proxy (GSoC 2019)
* And many many others.
