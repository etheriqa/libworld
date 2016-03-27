libworld
========

[![Build Status](https://travis-ci.org/etheriqa/libworld.svg?branch=master)](https://travis-ci.org/etheriqa/libworld)

**libworld** is a small toy library which provides an associative array with asynchronous replication to many clients.

Concepts
--------

- Adopt a simple master-slave topology (not multi-master, nor peer-to-peer).
- Aim at making a master be capable of replicating the data up to 1k slaves.
- Prefer stability and lowness of latency rather than throughput.
- Never implement an encryption layer, a compression layer, and so on.
- Never support the memcached protocols.
- Suppose a compiler complying with the C11 language standard (esp. Clang, GCC).
- Suppose a platform conforming to POSIX.1-2008 (esp. Linux, OS X).

API Overview
------------

See [include/world.h](include/world.h).
In addition, an end-to-end test ([test/e2e/world.c](test/e2e/world.c)) may help you to see.

Note that, however, the API is unstable and the implementation is highly error-prone at the present.

Benchmark
---------

All the following results are measured on OS X 10.11.3 with Intel Core i5-4250U.

### Throughput

#### 16 clients

```sh
% ./bench_log -s tcp -r 16
socket type (unix|tcp) (-s) ... tcp
port                   (-p) ... 25200
# of I/O threads       (-t) ... 1
# of replicas          (-r) ... 16
cardinality            (-c) ... 65536
key size               (-k) ... 16
data size              (-d) ... 64
watermark              (-w) ... 1024
seq 2088384-2089281, 280.7 MB/s, 3.341 M messages/s, 208.830 K op/s
seq 4163648-4164672, 278.9 MB/s, 3.321 M messages/s, 207.520 K op/s
seq 6191424-6192448, 272.4 MB/s, 3.243 M messages/s, 202.678 K op/s
```

#### 1024 clients

```sh
% ./bench_log -s tcp -r 1024
socket type (unix|tcp) (-s) ... tcp
port                   (-p) ... 25200
# of I/O threads       (-t) ... 1
# of replicas          (-r) ... 1024
cardinality            (-c) ... 65536
key size               (-k) ... 16
data size              (-d) ... 64
watermark              (-w) ... 1024
seq 34241-35279, 295.0 MB/s, 3.512 M messages/s, 3.423 K op/s
seq 68736-69767, 296.4 MB/s, 3.529 M messages/s, 3.449 K op/s
seq 103104-104142, 295.8 MB/s, 3.521 M messages/s, 3.437 K op/s
```

### Latency

WIP
