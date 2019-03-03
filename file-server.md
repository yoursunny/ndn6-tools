# file-server

`file-server` tool serves files from a directory.

## Usage

On a server, place files in a directory, and execute:

    ./file-server /prefix /directory

On a client, retrieve `/directory/file.txt` with:

    ndncatchunks -d realtime /prefix/file.txt > file.txt

