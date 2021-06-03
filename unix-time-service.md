# unix-time-service

`unix-time-service` tool answers queries of current Unix timestamp.

This is compatible with [esp8266ndn UnixTime client](https://github.com/yoursunny/esp8266ndn/blob/main/src/app/unix-time.hpp).

## Protocol

UnixTime query:

* Interest
* Name: `/localhop/unix-time`
* CanBePrefix: yes
* MustBeFresh: yes

UnixTime answer:

* Data
* Name: `/localhop/unix-time/`*timestamp*
* FreshnessPeriod: 1ms

## Usage

```bash
./unix-time-service
```
