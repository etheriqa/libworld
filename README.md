libworld
========

[![Build Status](https://travis-ci.org/etheriqa/libworld.svg?branch=master)](https://travis-ci.org/etheriqa/libworld)

**libworld** is a small toy library which provides an associative array with asynchronous replication to many clients.

Concepts
--------

- Adopt a simple master-slave topology. (neither multi-master, nor peer-to-peer)
- Aim at making a master be capable of replicating data up to 1k slaves.
- Prefer stability and lowness of latency rather than throughput.
- Assume a reliable network. (intended to do with TCP)
- Never implement an encryption layer, a compression layer, and so on.
- Never support the memcached protocols.
- Suppose a compiler complying with the C11 language standard. (esp. Clang, GCC)
- Suppose a platform conforming to POSIX.1-2008. (esp. Linux, OS X)

API Overview
------------

Refer to [include/world.h](include/world.h).

In addition, an end-to-end test ([test/e2e/world.c](test/e2e/world.c)) may help you to see.

Note, however, that the API is unstable and the implementation is highly error-prone at the present.
