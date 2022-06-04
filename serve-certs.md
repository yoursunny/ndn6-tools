# ndn6-serve-certs

`ndn6-serve-certs` tool is a producer for V2 certificates.
For each certificate, it registers a prefix derived from the certificate name minus IssuerId and Version components, which allows Interests of either key name or certificate name to reach this tool.

## Usage

Place BASE64 certificate files in a directory, and run:

```bash
ndn6-serve-certs --inter /path/to/*.ndncert
```

`--inter` flag requests the program to automatically gather and serve intermediate certificates.
This is useful if you want to serve the certificate chain.

## systemd Service

This tool can run as a systemd service.

```bash
# create the directory
sudo install -d -m0755 -ondn -gndn /var/lib/ndn/serve-certs

# add a certificate from user KeyChain
ndnsec cert-dump -i /U | sudo -u ndn tee /var/lib/ndn/serve-certs/U.ndncert >/dev/null

# add a certificate from NFD KeyChain (for prefix propagation)
sudo HOME=/var/lib/ndn/nfd -u ndn ndnsec cert-dump -i /U \
  | sudo -u ndn tee /var/lib/ndn/serve-certs/U.ndncert >/dev/null

# remove a certificate
sudo -u ndn rm /var/lib/ndn/serve-certs/U.ndncert

# start/restart the service
# (you must restart the service after adding/removing a certificate)
sudo systemctl restart ndn6-serve-certs

# make the service autostart with the system
sudo systemctl enable ndn6-serve-certs
```
