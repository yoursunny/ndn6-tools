# ndn6-prefix-allocate

`ndn6-prefix-allocate` tool allocates a prefix to the requesting face upon receiving a request.

## Deployment

On the router, execute:

```bash
ndn6-prefix-allocate /customer-prefix
```

Allocated prefixes will be under `/customer-prefix`, in the form of `ndn:/customer-prefix/<timestamp>_<faceId>`.  
A registered route will have origin 22804, and will not expire until requesting face is closed.

Logs are written to stdout in TSV format.
The columns are: timestamp, RibMgmt status code (0 for success), FaceId, prefix.

## Usage

From a remote host, express an Interest for `ndn:/localhop/prefix-allocate/<random>`.  
The response Data contains the allocated prefix in Content field as a Name element in TLV format.
