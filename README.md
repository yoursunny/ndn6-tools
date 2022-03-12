# ndn6-tools

[![CircleCI build status](https://img.shields.io/circleci/build/github/yoursunny/ndn6-tools)](https://app.circleci.com/pipelines/github/yoursunny/ndn6-tools) [![GitHub code size](https://img.shields.io/github/languages/code-size/yoursunny/ndn6-tools?style=flat)](https://github.com/yoursunny/ndn6-tools)

This repository contains tools running in [yoursunny ndn6 network](https://yoursunny.com/p/ndn6/).

## Available Tools

[ndn6-facemon](facemon.md): log when a face is created or destroyed

[ndn6-file-server](file-server.md): serve file from filesystem

[ndn6-prefix-allocate](prefix-allocate.md): allocate a prefix to requesting face

[ndn6-prefix-proxy](prefix-proxy.md): handle prefix registration command with prefix confinement

[ndn6-register-prefix-cmd](register-prefix-cmd.md): prepare a prefix registration command

[ndn6-register-prefix-remote](register-prefix-remote.md): send prefix registration commands to a specified remote face

[ndn6-serve-certs](serve-certs.md): serve certificates

[ndn6-unix-time-service](unix-time-service.md): answers queries of current Unix timestamp

## Install from Binary Package

[NFD-nightly](https://nfd-nightly.ndn.today/) publishes binary package `ndn6-tools`.

To install binary package:

1. Enable NFD-nightly APT repository.
2. `sudo apt install ndn6-tools`

## Install from Source

Requirements:

* Ubuntu 20.04, Debian 11
* [ndn-cxx](https://named-data.net/doc/ndn-cxx/) installed from PPA or [NFD-nightly](https://nfd-nightly.ndn.today/)

To compile and install:

```bash
make
sudo make install
```

To uninstall:

```bash
sudo make uninstall
```

## Development Container

The following commands prepare and start a Docker container with necessary tools for developing ndn6-tools.
It requires `run-ndn` volume shared with a separate NFD container, where NFD listens on Unix socket `/run/ndn/nfd.sock`.

```bash
docker build -t ndn6-tools-dev - <<EOT
FROM debian:bullseye
RUN apt-get update \
 && apt-get install -y --no-install-recommends build-essential ca-certificates clang-format-11 gdb git \
 && echo "deb [trusted=yes] https://nfd-nightly-apt.ndn.today/debian bullseye main" > /etc/apt/sources.list.d/nfd-nightly.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends libndn-cxx-dev ndn-dissect ndnchunks ndnpeek \
 && rm -rf /var/lib/apt/lists/* \
 && echo 'set enable-bracketed-paste off' >> /etc/inputrc \
 && mkdir -p /etc/ndn \
 && echo 'transport=unix:///run/ndn/nfd.sock' > /etc/ndn/client.conf
WORKDIR /app
EOT

docker run -it --rm --name ndn6-tools \
  --mount type=volume,source=run-ndn,target=/run/ndn \
  --mount type=bind,source=$(pwd),target=/app \
  --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
  ndn6-tools-dev
```
