# ndn6-tools

![CircleCI build status](https://img.shields.io/circleci/build/github/yoursunny/ndn6-tools) ![GitHub code size](https://img.shields.io/github/languages/code-size/yoursunny/ndn6-tools?style=flat)

This repository contains tools running in [yoursunny ndn6 network](https://yoursunny.com/p/ndn6/).

## Install from Binary Package

[NFD-nightly](https://nfd-nightly.ndn.today/) publishes binary package `ndn6-tools`.

To install binary package:

1. Enable NFD-nightly APT repository.
2. `sudo apt install ndn6-tools`

## Install from Source

`Makefile` assumes:

* OS is Ubuntu 18.04 or Ubuntu 20.04 or Debian 11
* [ndn-cxx](https://named-data.net/doc/ndn-cxx/) has been installed from PPA or [nightly](https://nfd-nightly.ndn.today/)

To compile and install:

```bash
make
sudo make install
```

To uninstall:

```bash
sudo make uninstall
```

## Available Tools

[facemon](facemon.md): log when a face is created or destroyed

[file-server](file-server.md): serve file from filesystem

[prefix-allocate](prefix-allocate.md): allocate a prefix to requesting face

[prefix-proxy](prefix-proxy.md): handle prefix registration command with prefix confinement

[prefix-request](prefix-request.md): register a prefix to requesting face, where the prefix is determined by a server process that knows a shared secret

[register-prefix-cmd](register-prefix-cmd.md): prepare a prefix registration command

[register-prefix-remote](register-prefix-remote.md): send prefix registration commands to a specified remote face

[serve-certs](serve-certs.md): serve certificates

[unix-time-service](unix-time-service.md): answers queries of current Unix timestamp
