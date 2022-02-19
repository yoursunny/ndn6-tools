# ndn6-register-prefix-remote

`ndn6-register-prefix-remote` tool periodically sends prefix registration commands to a specified remote face.
The signed Interests are sent with *NextHopFaceIdTag*, and does not follow the local FIB entry of `/localhop/nfd` prefix.
This enables two use cases:

* The local node can accept registrations from others at `/localhop/nfd` prefix, and still use this tool to send registrations to specified remote node(s).
* The local node can connect to multiple remote nodes, and send different registrations to each remote node.

Additionally, this tool can:

* Unregister unwanted prefixes created by `nfd-autoreg`.
* Readvertise prefixes found in NLSR NameLSA dataset.

## Usage

```bash
ndn6-register-prefix-remote -f udp6://[2001:db8:3de3:e486:ce0a:f157:c78c:b2e5]:6363 \
  -p /example/A -p /example/B \
  -i /com/example/user
```

* `-f` specifies remote FaceUri (required)
* `-p` specifies the prefix to be registered
* `-i` specifies signing identity (optional)
