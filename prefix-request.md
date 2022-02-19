# ndn6-prefix-request

`ndn6-prefix-request` tool registers a prefix to requesting face, where the prefix is determined by a server process that knows a shared secret.
This is a simplified alternate to remote prefix registration, suitable when prefix assignment is centrally controlled.

## Deployment

On the router, execute:

```bash
ndn6-prefix-request 63fyEiFW
```

"63fyEiFW" in the example above is the shared secret.  
You can generate a random secret with `makepasswd` command.

A registered route will have origin 19438, and will not expire until requesting face is closed.

Logs are written to stdout in TSV format.
The columns are: timestamp, RibMgmt status code (0 for success, 8401 for wrong answer), FaceId, prefix.

## Usage

On a web server, construct a request Name:

```text
ndn:/localhop/prefix-request/<prefix-uri>/<answer>/<random>
```

* `<prefix-uri>` is the URI representation of the prefix to be registered.  
  It's intentional to use URI instead of TLV, so that the server process doesn't need to understand NDN encoding.
* `<answer>` is calculated as `SHA256(<secret> <prefix-uri>)`, and represented as upper case hexadecimal.
* `<random>` is some random bytes.

Example code for PHP:

```php
define('SECRET', '63fyEiFW');

function prefix_request_interest($prefix) {
  $hashctx = hash_init('sha256');
  hash_update($hashctx, SECRET);
  hash_update($hashctx, $prefix);
  $answer = hash_final($hashctx);

  return sprintf('/localhop/prefix-request/%s/%s/%d', urlencode($prefix), strtoupper($answer), mt_rand());
}
```

From a remote host (eg. web browser), obtain the request Name from web server (eg. via HTTP), and express an Interest using this Name.  
The response Data contains the registered prefix in Content field as a Name element in TLV format.
