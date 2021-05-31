# file-server

`file-server` tool serves files from a directory.

## Usage

### Start a Server

```bash
ndn6-file-server /prefix /directory
```

### List Directory

To list directory `/directory/subdir`:

```bash
ndncatchunks -q /prefix/subdir/32=ls | tr '\0' '\n'
```

Response payload contains file and directory names separated by `\0`.
Having `/` at the end of a name indicates a directory.
Filesystem objects other than files and directories are skipped.

### Retrieve File

To retrieve file `/directory/subdir/file.txt`:

```bash
ndncatchunks -q /prefix/subdir/file.txt > file.txt
```
