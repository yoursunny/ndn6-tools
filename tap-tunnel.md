# tap-tunnel

`tap-tunnel` tool creates an Ethernet tunnel over NDN.

## Usage

Execute on first endpoint:

    sudo ip tuntap add dev tap0 mode tap user $(id -u)
    sudo ip link set tap0 up
    sudo ip addr add 192.168.41.0/31 dev tap0
    ./tap-tunnel -l /prefix-of-first-endpoint -r /prefix-of-second-endpoint -i tap0

Execute on second endpoint:

    sudo ip tuntap add dev tap0 mode tap user $(id -u)
    sudo ip link set tap0 up
    sudo ip addr add 192.168.41.1/31 dev tap0
    ./tap-tunnel -l /prefix-of-second-endpoint -r /prefix-of-first-endpoint -i tap0

Test on first endpoint:

    ping 192.168.41.1

## How Fast is tap-tunnel?

### ping

IP ping over `tap-tunnel` (2000 pings at 200ms interval)

    2000 packets transmitted, 1999 received, 0% packet loss, time 402824ms
    rtt min/avg/max/mdev = 47.367/51.563/111.025/3.058 ms

Compare to `ndnping` (2000 probes at 200ms interval)

    2000 packets transmitted, 2000 received, 0 nacked, 0% lost, 0% nacked, time 94811.5 ms
    rtt min/avg/max/mdev = 44.596/47.4058/95.8826/2.52611 ms

### download

HTTP `wget` over `tap-tunnel` (served from `python3 -m http.server`)

    10.00M   110KB/s    in 93s

Compare to `scp` (similar network path)

    10MB 682.7KB/s   00:15
