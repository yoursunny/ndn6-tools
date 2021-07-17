# prefix-proxy

`prefix-proxy` tool receives prefix registration commands on `/localhop/nfd/rib/register` and `/localhop/nfd/rib/unregister` prefixes, and proxies them to NFD management under `/localhost/nfd` prefix.
It performs additional validation that confines the registered prefix within the identity name of the signing key.

## Usage

```bash
ndn6-prefix-proxy --anchor root.ndncert.base64 --open-prefix /ndn/multicast
```

* `--anchor` specifies a trust anchor file (required, repeatable)
* `--open-prefix` specifies a prefix that anyone with a valid certificate can register without being confined by identity name (optional, repeatable)
