# file-server

`file-server` tool serves files from a directory.
This is compatible with [NDNts](https://yoursunny.com/p/NDNts/) `@ndn/cat` package `ndncat file-client` command.

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

## Protocol Details

Both directory listing and file retrieval start with a [RDR discovery Interest](https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR).
The file server responds with a metadata packet that contains the current version of the file or directory, in which the version number is derived from last modification timestamp.

RDR metadata packets are encoded with extension fields that may help the consumer:

```abnf
NDN6-FILE-SERVER-METADATA = NDN6-FILE-SERVER-DIR-METADATA |
                            NDN6-FILE-SERVER-FILE-METADATA

NDN6-FILE-SERVER-DIR-METADATA =
  Name ; versioned name

NDN6-FILE-SERVER-FILE-METADATA =
  Name ; versioned name
  ByteOffsetNameComponent ; file size in bytes
  SegmentNameComponent ; last segment number (inclusive)
```

Then, the consumer downloads the directory listing or file content as a segmented object under the discovered version.
Version and segment components are encoded as [Naming Conventions rev2](https://named-data.net/publications/techreports/ndn-tr-22-2-ndn-memo-naming-conventions/).
*FinalBlockId* in every segment packet points to the last segment number.

If the request is invalid, such as nonexisting path, incorrect version number (differs from last modification timestamp), "ls" on a file, the server responds with an application layer Nack (Data packet with ContentType=Nack).
The consumer should be prepared to handle this condition.
