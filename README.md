# ndn6-tools

This repository contains tools for yoursunny's (now discontinued) NDN6 router.

## Build Instructions

`Makefile` assumes:

* OS is Ubuntu 18.04 or 20.04
* [ndn-cxx](https://named-data.net/doc/ndn-cxx/) has been installed from PPA

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

[prefix-request](prefix-request.md): register a prefix to requesting face, where the prefix is determined by a server process that knows a shared secret

[register-prefix-cmd](register-prefix-cmd.md): prepare a prefix registration command

[serve-certs](serve-certs.md): serve certificates

[unix-time-service](unix-time-service.md): answers queries of current Unix timestamp
