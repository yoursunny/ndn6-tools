# ndn6-file-server

`ndn6-file-server` tool serves files from a directory.
This is compatible with [NDNts](https://yoursunny.com/p/NDNts/) `@ndn/cat` package `ndncat file-client` command.

## Usage

### Start a Server

```bash
ndn6-file-server -b /prefix -d /directory

ndn6-file-server -D /prefix -b /replica/prefix -d /directory
```

* `--listen` or `-b` specifies the serve prefix (required).
  The prefix should consist of only GenericNameComponents.
  Both metadata and segments are served under this prefix.
* `--discovery` or `-D` specifies a discovery prefix (optional).
  The prefix should consist of only GenericNameComponents.
  Only metadata packets are available under this prefix.
  The `Name` field in the metadata points to the serve prefix.
* `--directory` or `-d` specifies a local directory (required).
  Contents of this directory are made available by the file server.
  This must be an absolute path.
* `--segment-size` or `-s` specifies the segment length (optional, defaults to 6144).
  This shall be an integer between 1 and 8192.
  Since segment packets can be cached, you should not change this setting after the file server is in operation.

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

### RDR Discovery

The consumer first sends a [RDR discovery Interest](https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR):

* `/prefix/subdir/32=ls/32=metadata`: directory listing
* `/prefix/subdir/file.txt/32=metadata`: file retrieval
* `/prefix/subdir/32=metadata`: refer to a directory without "32=ls" keyword

The prefix portion may be either the discovery prefix or the serve prefix of the file server. 
The Interest must have CanBePrefix and MustBeFresh, as per RDR specification.

### RDR Metadata

The file server responds with a metadata packet.
Its Content payload contains the following TLV elements:

* Name: versioned name prefix; version number is derived from last modification time.
* FinalBlockId: enclosed SegmentNameComponent that reflects last segment number (inclusive).
* SegmentSize (TLV-TYPE 0xF500, NonNegativeInteger): segment size (octets); last segment may be shorter.
* Size (TLV-TYPE 0xF502, NonNegativeInteger): file size (octets).
* Mode (TLV-TYPE 0xF504, NonNegativeInteger): file type and mode, see [inode manpage](https://man7.org/linux/man-pages/man7/inode.7.html).
* Atime (TLV-TYPE 0xF506, NonNegativeInteger): last accessed time (nanoseconds since Unix epoch).
* Btime (TLV-TYPE 0xF508, NonNegativeInteger): creation time (nanoseconds since Unix epoch).
* Ctime (TLV-TYPE 0xF50A, NonNegativeInteger): last status change time (nanoseconds since Unix epoch).
* Mtime (TLV-TYPE 0xF50C, NonNegativeInteger): last modification time (nanoseconds since Unix epoch).

Name, Mode, Mtime are always present.
FinalBlockId, SegmentSize, Size are omitted on a directory.
Atime, Btime, Ctime may be omitted if the underlying filesystem cannot provide them.

The TLV elements may appear in any order.
The consumer should ignore any TLV element with an unrecognized TLV-TYPE.

### Segmented Object Retrieval

Then, the consumer downloads the directory listing or file content as a segmented object under the discovered version.
Version and segment components are encoded as [Naming Conventions rev3](https://named-data.net/publications/techreports/ndn-tr-22-3-ndn-memo-naming-conventions/).
*FinalBlockId* in every segment packet points to the last segment number.

### Error Handling

If the request is invalid, such as nonexisting path, incorrect version number (differs from last modification timestamp), "ls" on a file:

* For an RDR discovery Interest, the server responds with an application layer Nack (Data packet with ContentType=Nack).
* For a segment Interest, the server does not respond.

The consumer should be prepared to handle this condition.
