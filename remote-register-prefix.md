# remote-register-prefix

`remote-register-prefix` tool sends a remote prefix registration command to a specific gateway router.
Unlike built-in remote prefix registration in NFD RIB daemon, this tool allows specifying signing identity and registered prefix independently.

## Usage

On the laptop, execute:

    ./remote-register-prefix -f udp4://192.0.2.1:6363 -p ndn:/my-prefix -i ndn:/signing-identity

* `-f` specifies FaceUri (required; must be canonical FaceUri)
* `-p` specifies the prefix to be registered (required)
* `-i` specifies signing identity (optional)
