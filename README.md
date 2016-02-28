libworld
========

**libworld** is an experimental library which provides a simple way of replicating a key-value storage over reliable network.

Features (or what to be aimed)
------------------------------

- Continuously replicating a key-value storage to massively many clients in low latency.
- Taking a snapshot with O(1) time complexity which is independent of a number of entries.
