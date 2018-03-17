# serve-certs

`serve-certs` tool serves V2 certificates.
For each certificate, it registers a prefix derived from the certificate name minus IssuerId and Version components.
This allows Interests expressed with KeyLocatorName to reach this tool.

## Usage

On a server, place BASE64 certificate files in a directory, and execute:

    ./serve-certs /path/to/*.cert

## systemd Service

To use the systemd service:

1. Place `serve-certs.service` to `/lib/systemd/system/serve-certs.service`.
2. `sudo mkdir /var/lib/ndn/serve-certs`.
3. Place one or more BASE64-encoded certificates (`*.ndncert`) into `/var/lib/ndn/serve-certs` directory.
4. `sudo chown -R ndn:ndn /var/lib/ndn/serve-certs`.
5. `sudo systemd start serve-certs`.
