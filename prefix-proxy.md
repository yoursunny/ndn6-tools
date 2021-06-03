# prefix-proxy

`prefix-proxy` tool receives prefix registration commands on `/localhop/nfd/rib/register` and `/localhop/nfd/rib/unregister` prefixes, and proxies them to NFD management under `/localhost/nfd` prefix.
It performs additional validation that confines the registered prefix within the identity name of the signing key.

## Usage

```bash
ndn6-prefix-proxy --anchor root.ndncert.base64
```

* `--anchor` specifies a trust anchor file (repeatable)
