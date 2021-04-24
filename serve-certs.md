# serve-certs

`serve-certs` tool is a producer for V2 certificates.
For each certificate, it registers a prefix derived from the certificate name minus IssuerId and Version components, which allows Interests of either key name or certificate name to reach this tool.

## Usage

On a server, place BASE64 certificate files in a directory, and execute:

```bash
ndn6-serve-certs /path/to/*.ndncert
```

## systemd Service

This tool can run as a systemd service.

```bash
# create the directory
sudo install -d -m0755 -ondn -gndn /var/lib/ndn/serve-certs

# add a certificate
ndnsec cert-dump -i /U | sudo -u ndn tee /var/lib/ndn/serve-certs/U.ndncert >/dev/null

# remove a certificate
sudo -u ndn rm /var/lib/ndn/serve-certs/U.ndncert

# start/restart the service
# (you must restart the service after adding/removing a certificate)
sudo systemctl restart ndn6-serve-certs

# make the service autostart with the system
sudo systemctl enable ndn6-serve-certs
```
