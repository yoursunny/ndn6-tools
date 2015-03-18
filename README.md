# ndn6: tools

This repository contains tools running on [NDN6](http://yoursunny.com/p/ndn6/) router.

## Build Instructions

`Makefile` assumes:

* OS is Ubuntu 14.04 32-bit
* [ndn-cxx](http://named-data.net/doc/ndn-cxx/) has been installed from source code

To compile and install:

    make
    sudo make install

To uninstall:

    sudo make uninstall

## Available Tools

[prefix-allocate](prefix-allocate.md): allocate a prefix to requesting face
