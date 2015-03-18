# facemon

`facemon` tool prints a log line whenever a face is created or destroyed.
It's meant to be piped into a script that takes action according to those events.

## Deployment

On the router, execute:

    ./facemon

Logs are written to stdout in TSV format.
The columns are: timestamp, event ("CREATED" or "DESTROYED"), FaceId, RemoteUri, LocalUri.

`facemon` also prints a log line when an Interest under prefix `ndn:/localhop/facemon` is received.
The columns are: timestamp, "INTEREST", FaceId, then one column for each component except the first two.  
This is useful for an app to say something to the script watching `facemon` output; those Interests will not be answered.
