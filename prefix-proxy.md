# ndn6-prefix-proxy

`ndn6-prefix-proxy` tool receives prefix registration commands on `/localhop/nfd/rib/register` and `/localhop/nfd/rib/unregister` prefixes, and proxies them to NFD management under `/localhost/nfd` prefix.
It additionally confines the registered prefix within the identity name of the signing key.

## Usage

```bash
ndn6-prefix-proxy --anchor root.ndncert.base64 --open-prefix /ndn/multicast
```

* `--anchor` specifies a trust anchor file (required, repeatable)
* `--open-prefix` specifies a prefix that anyone with a valid certificate can register without being confined by identity name (optional, repeatable)

## NFD Configuration for RIB Dataset

This tool does not publish RIB dataset on `/localhop/nfd/rib/list` prefix.
To make NFD publish RIB dataset without accept unconfined prefix registration commands, modify NFD configuration as follows:

```bash
infoedit -f nfd.conf -d rib.auto_prefix_propagate
infoedit -f nfd.conf -r rib.localhop_security <<EOT
  rule
  {
    id x
    for data
    checker
    {
      type hierarchical
      sig-type ecdsa-sha256
    }
  }
  trust-anchor
  {
    type dir
    dir /nonexistent
  }
EOT
```
