# tinySSB Simple Chat

SimpleChat is a text-user-interface app for reading and writing public
chat messages in tinySSB. It serves mostly as a testing tool.

The main program is ```simplechat.py```. It is built on top of
SimplePub, hence shares several configuration options with
it. Internal logging messages can be seen by toggeling with ESC the
screen between the chat and log view.

In comparison to SimplePub, SimpleChat has a notion of authorship. If
not existing, an ED25519 private key is generated and the corresponding
append-only log is added to the repository of the unterlying
SimplePub.  Authorship permits to create and sign new log entries,
which SimplChat requires in order to post messages.

## Description

"tinySSB simple chat" displays all public 'Text-And-Voice' messages found in
a repo's content; new messages can be added. The replication logic running underneath SimpleChat:
- is accessible via web sockets, and
- is accessible via BLE (central-only)
- runs the tinySSB synchronization protocol
  -  datagram-based, packets have 120 Bytes or less
  -  growOnlySet protocol for compressing feed IDs
  -  vectors with WANT and CHNK information
- has adaptive timers, made for reliable connections
- is crash resistant: the ```frontier.bin``` file for a log is updated on startup, should the log have been extended but the frontier failed to be updated

See the README.md of SimplePub for the limitation.

A log entry for the ```TAV``` public chat of tinySSB has the following
format:

```
[ "TAV",            # identifies the app
  "a text string",  # the posted text
  #c0dec2010..#,    # codec2-encoded voice, or null
  1691098012 ]      # timestamp
```

This list of four values is BIPF-encoded and stored in the tinySSB
append-only log as a "chain20" log entry type.


## ```simplechat.py``` - a p2p tinySSB chat client

```
usage: simplechat.py [-h] [-data DATAPATH] [-id IDPATH] [-role {in,inout,out}] [-v] [uri_or_port]

positional arguments:
  uri_or_port           TCP port if responder, URI if intiator (default is ws://127.0.0.1:8080)

options:
  -h, --help            show this help message and exit
  -ble                  enable Bluetooth Low Energ (default: off)
  -data DATAPATH        path to persistency directory (default: ./data)
  -id IDPATH            path to tinySSB private directory (default: ~/.tinySSB)
  -role {in,inout,out}  direction of data flow (default: inout)
  -v                    print i/o timestamps
```

Example how Alice and Bob start their chat clients (on the same machine):
```
% ./simplechat.py -d ./alice -i ./alice -v 8080             # Alice' clients will respond on websocket port 8080

% ./simplechat.py -d ./bob -i ./bob -v ws://127.0.0.1:8080  # Bob is initiator
```

## Screenshots

The screen of Bob, before sending the reply:
```
-────────── tinyS─────-- <w> connection up─┐
│                                          │
│ #19 [TVUK3-ZVR4B]                        │
│ Heyo local butts, what's going on!       │
│ _                 ⏲ 2023-06-24 19:48:45  │
│                                          │
│ #20 [6F24K-PBSQ6]                        │
│ <voice msg>                              │
│ _                 ⏲ 2023-06-26 01:10:18  │
│                                          │
│ #21 [73ZOO-QOWFW]                        │
│ Message                                  │
│ _                 ⏲ 2023-07-04 07:50:33  │
│                                          │
│ #22 [QDS2Z-Z3U6W]                        │
│ Message                                  │
│ _                 ⏲ 2023-07-04 08:02:44  │
│                                          │
│ #23 [7NQQX-YZUDL]                        │
│ Hi Bob, how are you? Cheers, Alice       │
│ _                 ⏲ 2023-08-03 23:22:45  │
│                                          │
┌────────────────────────────────────./bob─┐
│> Hi Alice, all is fine. Best, Bob        │
└─────────────────── ESC=log/chat ^C=exit ─┘
```

The screen of Alice, after having received the reply:
```
/────────── tinyS─────────────sending 120B─┐
│                                          │
│ #20 [6F24K-PBSQ6]                        │
│ <voice msg>                              │
│ _                 ⏲ 2023-06-26 01:10:18  │
│                                          │
│ #21 [73ZOO-QOWFW]                        │
│ Message                                  │
│ _                 ⏲ 2023-07-04 07:50:33  │
│                                          │
│ #22 [QDS2Z-Z3U6W]                        │
│ Message                                  │
│ _                 ⏲ 2023-07-04 08:02:44  │
│                                          │
│ #23 [7NQQX-YZUDL]                        │
│ Hi Bob, how are you? Cheers, Alice       │
│ _                 ⏲ 2023-08-03 23:22:45  │
│                                          │
│ #24 [LJUGR-FULBS]                        │
│ Hi Alice, all is fine. Best, Bob         │
│ _                 ⏲ 2023-08-03 23:26:52  │
│                                          │
┌──────────────────────────────────./alice─┐
│>                                         │
└─────────────────── ESC=log/chat ^C=exit ─┘
```


Showing the log view (switching is done with ESC)
```
-────^[──── tinyS─────-- <w> connection up─┐
│                                          │
│ gset dmx 613dfa70c47aba                  │
│ want dmx 343fc019f3c9ad                  │
│ chnk dmx 2af30fda04c000                  │
│ Starting websocket responder on port 808 │
│  w> 0.000 o=1     61B 0x343fc019f3c9ada40│
│  <w 0.001 i=2     22B 0x2af30fda04c000743│
│  <w 0.001 i=3    105B 0x613dfa70c47aba631│
│  w> 4.002 o=4     61B 0x343fc019f3c9ada40│
│  <w 4.003 i=5     22B 0x2af30fda04c000743│
│     new c=[ 1.6.32 1.7.0 ]               │
│  w> 8.004 o=6     61B 0x343fc019f3c9ada40│
│  <w 8.007 i=7     22B 0x2af30fda04c000743│
│     new c=[ 1.6.32 1.7.0 ]               │
│  w> 12.006 o=8     61B 0x343fc019f3c9ada4│
│  <w 12.007 i=9     22B 0x2af30fda04c00074│
│  <w 15.002 i=10   105B 0x613dfa70c47aba63│
│  w> 16.008 o=11    61B 0x343fc019f3c9ada4│
│  <w 16.010 i=12    22B 0x2af30fda04c00074│
│     new c=[ 1.6.32 1.7.0 ]               │
│     =C [ 1.6.32 1.7.0 ] []               │
┌──────────────────────────────────./alice─┐
│>                                         │
└─────────────────── ESC=log/chat ^C=exit ─┘
```

---
