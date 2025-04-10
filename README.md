# ndn6-tools

[![CircleCI build status](https://img.shields.io/circleci/build/github/yoursunny/ndn6-tools?style=flat)](https://app.circleci.com/pipelines/github/yoursunny/ndn6-tools) [![GitHub code size](https://img.shields.io/github/languages/code-size/yoursunny/ndn6-tools?style=flat)](https://github.com/yoursunny/ndn6-tools)

This repository contains tools running in [yoursunny ndn6 network](https://yoursunny.com/p/ndn6/).

![ndn6-tools logo](logo.svg)

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

* Ubuntu 22.04, Ubuntu 24.04, Debian 12
* [ndn-cxx](https://named-data.net/doc/ndn-cxx/) installed from source or [NFD-nightly](https://nfd-nightly.ndn.today/) `libndn-cxx-dev` package

To compile and install:

```bash
make
sudo make install
```

To uninstall:

```bash
sudo make uninstall
```
