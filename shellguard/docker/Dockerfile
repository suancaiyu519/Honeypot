# This Dockerfile contains two images, `builder` and `runtime`.
# `builder` contains all necessary code to build
# `runtime` is stripped down.

ARG SOURCE_DATE_EPOCH
FROM debian:bookworm-slim AS builder

WORKDIR /

ENV SHELLGUARD_GROUP=shellguard \
    SHELLGUARD_USER=shellguard \
    SHELLGUARD_HOME=/shellguard

# Set locale to UTF-8, otherwise upstream libraries have bytes/string conversion issues
ENV LC_ALL=en_US.UTF-8 \
    LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8

RUN groupadd -r ${SHELLGUARD_GROUP} && \
    useradd -r -d ${SHELLGUARD_HOME} -m -g ${SHELLGUARD_GROUP} ${SHELLGUARD_USER}

# Set up Debian prereqs
RUN export DEBIAN_FRONTEND=noninteractive; \
    apt-get -qq update && \
    apt-get -qq install -y \
    -o APT::Install-Suggests=false \
    -o APT::Install-Recommends=false \
    -o Dpkg::Use-Pty="0" \
    build-essential \
    ca-certificates \
    cargo \
    libffi-dev \
    libsnappy-dev \
    libssl-dev \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    rustc && \
    rm -rf /var/lib/apt/lists/*

USER ${SHELLGUARD_USER}
WORKDIR ${SHELLGUARD_HOME}

# Copy requirements first to use Docker caching better
RUN mkdir -p ${SHELLGUARD_HOME}/shellguard-git
COPY --chown=${SHELLGUARD_USER}:${SHELLGUARD_GROUP} requirements.txt requirements-output.txt ${SHELLGUARD_HOME}/shellguard-git/

RUN python3 -m venv shellguard-env && \
    . shellguard-env/bin/activate && \
    pip install -q --no-cache-dir --upgrade pip setuptools wheel && \
    pip install -q --no-cache-dir --upgrade cffi && \
    pip install -q --no-cache-dir --upgrade -r ${SHELLGUARD_HOME}/shellguard-git/requirements.txt && \
    pip install -q --no-cache-dir --upgrade -r ${SHELLGUARD_HOME}/shellguard-git/requirements-output.txt

COPY --chown=${SHELLGUARD_USER}:${SHELLGUARD_GROUP} . ${SHELLGUARD_HOME}/shellguard-git

# Set version for setuptools-scm since .git directory is not included
ENV SETUPTOOLS_SCM_PRETEND_VERSION=1.0.0

# Install ShellGuard in development mode
RUN . shellguard-env/bin/activate && \
    pip install -e ${SHELLGUARD_HOME}/shellguard-git


FROM gcr.io/distroless/python3-debian12:latest AS runtime
#FROM gcr.io/distroless/python3-debian12:debug AS runtime

LABEL org.opencontainers.image.title="ShellGuard SSH/Telnet Honeypot"
LABEL org.opencontainers.image.description="ShellGuard - SSH/Telnet 蜜罐系统"
LABEL org.opencontainers.image.vendor="ShellGuard"
LABEL org.opencontainers.image.licenses="BSD-3-Clause"
LABEL org.opencontainers.image.base.name="gcr.io/distroless/python3-debian12"
# 如需配置仓库信息，请取消注释并填写实际地址:
# LABEL org.opencontainers.image.source="https://github.com/YOUR_USERNAME/shellguard"
# LABEL org.opencontainers.image.url="https://your-domain.com"
# LABEL org.opencontainers.image.documentation="https://your-docs.com"

ENV SHELLGUARD_GROUP=shellguard \
    SHELLGUARD_USER=shellguard \
    SHELLGUARD_HOME=/shellguard

#RUN groupadd -r ${SHELLGUARD_GROUP} && \
#    useradd -r -d ${SHELLGUARD_HOME} -m -g ${SHELLGUARD_GROUP} ${SHELLGUARD_USER}
COPY --from=builder --chown=0:0 /etc/passwd /etc/group /etc/

#RUN export DEBIAN_FRONTEND=noninteractive; \
#    apt-get update && \
#    apt-get install -y \
#        -o APT::Install-Suggests=false \
#        -o APT::Install-Recommends=false \
#        -o Dpkg::Use-Pty="0" \
#      libssl1.1 \
#      ca-certificates \
#      libffi7 \
#      procps \
#      python3 \
#      python3-distutils && \
#    rm -rf /var/lib/apt/lists/* && \
#    ln -s /usr/bin/python3 /usr/local/bin/python

COPY --from=builder --chown=${SHELLGUARD_USER}:${SHELLGUARD_GROUP} ${SHELLGUARD_HOME} ${SHELLGUARD_HOME}

RUN [ "python3", "-m", "compileall", "-q", "/shellguard/shellguard-git/src", "/shellguard/shellguard-env/", "/usr/lib/python3.11"]

VOLUME [ "/shellguard/shellguard-git/var", "/shellguard/shellguard-git/etc" ]

USER ${SHELLGUARD_USER}
WORKDIR ${SHELLGUARD_HOME}/shellguard-git

ENV PATH=${SHELLGUARD_HOME}/shellguard-env/bin:${PATH}
ENV PYTHONPATH=${SHELLGUARD_HOME}/shellguard-git/src
ENV PYTHONUNBUFFERED=1

RUN [ "python3", "/shellguard/shellguard-git/bin/regen-dropin.cache" ]

ENTRYPOINT [ "/shellguard/shellguard-env/bin/python3" ]
CMD [ "/shellguard/shellguard-env/bin/twistd", "-n", "--umask=0022", "--pidfile=", "shellguard" ]

EXPOSE 2222 2223
