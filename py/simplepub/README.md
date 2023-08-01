# tinySSB Simple Pub

The three programs of this repo are:
- [spub.py](#spubpy---a-pure-peer-pub-can-be-both-initiator-and-responder) the websocket and BlueTooth Low Energy server
- [start.sh](#startsh---a-bash-script-for-launching-the-websocket-server) a simple Bash script for launching the server on port 8080
- [frontier.py](#frontierpy---displays-the-content-of-the-persistence-directory-including-un-bipf-ing-where-possible) displays the server's content

See the ```simplepub``` directory for the self-contained library (no
external dependencies).


## Description

The "tinySSB simple pub" offers an Internet-based storage area for tinySSB append-only logs:
- accessible via web sockets, and
- accessible via BLE (central-only)
- running the tinySSB synchronization protocol
  -  datagram-based, packets have 120 Bytes or less
  -  growOnlySet protocol for compressing feed IDs
  -  vectors with WANT and CHNK information
- adaptive timers, made for reliable connections
- crash resistant: the ```frontier.bin``` file for a log is updated on startup, should the log have been extended but the frontier failed to be updated

The simple pub lacks:
- metadata privacy (no secure handshake protocol in place)
- access control (no read-only version: every device can add to the grow-only set)
- feed management (removing obsolete feeds)
- per end-device grow-only set (only one global table of feed IDs)
- resilience (DHT-like self-reconfiguration)

Compared to previous TinySSB relays, the software has been heavily rewritten:
- new experimental file system layout: only two files per feed (2FPF)
  - the ```log.bin``` file contains for each entry its full length sidechain even if not all chunks have been received yet
  - the ```frontier.bin``` file stores essential properties, including a list of unfinished sidechains
  - by persisting the sidechain status in ```frontier.bin``` we avoid a rescan of the file system at startup
- no main loop anymore for handling IO: Instead we adopt the asyncio model of the ```websockets``` package and have three different tasks:
  - goset
  - WANT vector
  - CHNK vector
- no dependency on other tinySSB packages: BIPF and pure25519 are included

## Programs

### ```spub.py``` - a pure peer pub: can be both initiator and responder

```
usage: spub.py [-h] [-d DATAPATH] [-role {in,inout,out}] [-v] [uri_or_port]

positional arguments:
  uri_or_port           TCP port if responder, URI if intiator (default is ws://127.0.0.1:8080)

options:
  -h, --help            show this help message and exit
  -ble                  enable Bluetooth Low Energ (default: off)
  -data DATAPATH        path to persistency directory (default: ./data)
  -role {in,inout,out}  direction of data flow (default: in)
  -v                    print i/o timestamps
```

Examples for starting the tinySSB SimplePub
```
% ./spub.py -r out 8080                    # read-only responder on websocket port 8080

% ./spub.py -r inout 8080                  # fully replicating pub responder

% ./spub.py -d data2 ws://127.0.0.1:8080   # download-only initiator
```


### ```frontier.py``` - displays the content of the persistence directory (including un-BIPF-ing where possible)

```
usage: frontier.py [-h] [-d DATAPATH] [-s]

options:
  -h, --help   show this help message and exit
  -d DATAPATH  path to persistency directory
  -s           only show stats (no content), default: False
```

Example
```
% ./frontier.py -d data -s
Stats:
- 23 feeds
- 147 available entries
- 261 available chunks
- 36 missing chunks: 1.6.32ff, 1.7.0ff
```

### ```start.sh``` - a Bash script for launching the websocket server

```
# launches the websocket server on port 8080 in a loop
# (restarting the server after 10 sec if it crashes)

% ./start.sh
```


## Notes

Because of our "no external dependencies" approach we include the
Bleak library from July 2023 (release v0.20.2) as is. See
[https://github.com/hbldh/bleak](https://github.com/hbldh/bleak) for
the latest version.

----
