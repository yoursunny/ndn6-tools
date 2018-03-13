# serve-certs

`serve-certs` tool serves V2 certificates.
For each certificate, it registers a prefix derived from the certificate name minus IssuerId and Version components.
This allows Interests expressed with KeyLocatorName to reach this tool.

## Usage

On a server, place BASE64 certificate files in a directory, and execute:

    ./serve-certs /path/to/*.cert
