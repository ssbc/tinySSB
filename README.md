# tinySSB - the LoRa descendant of Secure Scuttlebutt

![tinySSB logo](doc/_img/tinySSB-banner.png)

## Overview

tinySSB is a variation of Secure Scuttlebutt (SSB)
[https://scuttlebutt.nz/](https://scuttlebutt.nz/), a radically
decentral approach of implementing distributed applications. In a
nutshell, with SSB we get "social media without servers".

## How it works

The core of SSB are single-author append-only logs that are replicated
at will and on a best-effort basis. Eventually, all updates to a log
will reach the interested parties who can verify the authenticity and
integrity of each update.  This also applies to auxiliary forwarders,
making any SSB-aware entity a potential link in the forwarding chains.
Any means of replication is fine, which can be BlueTooth Low Energy,
Internet protocols, USB sticks, or data printed on paper, collected
and dispatched centrally or simply using a gossip protocol: anything
goes!

Each append-only log is a trivial _Conflict-free Replicated Data Type_
(CRDT). A set of append-only logs also of a CRDT. CRDTs are best
understood as virtual (because distributed) data where a local site
keeps a copy and can act on this copy. CRDTs are designed such that
any local modification results in update messages sent to the other
replicas: if these updates are ingested correctly, all replicas will
converge to the same shared state, without any central entity having
to intervene or help. Writing distributed applications as and with
CRDTs requires careful design but has a huge reward in unbounded
scallability, in applications that continue to work when the device is
offline because all updates seamlessly merge into the other replicas
when the device reconnects. Go decentral!


## The difference between SSB and tinySSB

tinySSB inherits the core concepts of "classic" SSB i.e., the
append-only logs with signed entries, the encryption suite etc.  The
novelty of tinySSB lies in its

- binary packet format (instead of JSON)
- “shadow packet headers” that avoid sending and storing redundant data
- the absence of “blobs” outside the append-only logs, using side chains instead

The data packet format has been made extremely small, namely 120
Bytes.  The point is to enable the use of tinySSB in challenged
environments where bandwidth and storage resources are scarce.

tinySSB runs over Bluetooth Low Energy (BLE), over long-range radio
(LoRA), and perhaps in the future even over shortwave in the amateur
radio bands, bouncing off the ionosphere. Embedded devices are
powerful enough to handle these packets and serve well as cheap
forwarders.

At the higher level, the design of distributed applications over
tinySSB as well as SSB is identical - it's a CRDT world.

tinySSB is used for teaching distributed programming concepts and
skills at the Computer Science Bachelor and Masters level at the
University of Basel.


## tinySSB Tech Gallery

(see the respective folders)

- Android
- ESP32



## Documentation

-- 16 factsheets made for the [dWeb camp 2024](https://dwebcamp.org/), Aug 2024: [PDF](doc/tinySSB-factsheets-v2b.pdf)

---
