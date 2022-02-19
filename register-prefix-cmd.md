# ndn6-register-prefix-cmd

`ndn6-register-prefix-cmd` tool prepares a prefix registration command, and writes the signed command to stdout.
One use case is to generate a command on behalf of a less powerful sensor device.

## Usage

On a server, execute:

```bash
ndn6-register-prefix-cmd -p /com/example/user/sensor1 -C -i /com/example/user
```

* `-p` specifies the prefix to be registered (required)
* `-C` enables Capture flag
* `-i` specifies signing identity (optional)

The binary output on stdout can then be delivered to the sensor device out-of-band (eg. through HTTP).
Afterwards, the sensor device can send the command to a remote NDN router.
