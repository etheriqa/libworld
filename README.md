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

## 0 clients

```sh
% ./bench_concurrent -r 0
# of I/O threads (-t) ... 1
# of replicas    (-r) ... 0
cardinality      (-c) ... 65536
key size         (-k) ... 16
data size        (-d) ... 64
watermark        (-w) ... 1024
seq 14571970-14571970, 0.0 MB/s, 0.000 M messages/s, 1456.412 K op/s
seq 29362144-29362145, 0.0 MB/s, 0.000 M messages/s, 1478.687 K op/s
seq 44044247-44044247, 0.0 MB/s, 0.000 M messages/s, 1468.083 K op/s
```

## 16 clients, UNIX-domain socket

```sh
% ./bench_concurrent -r 16
# of I/O threads (-t) ... 1
# of replicas    (-r) ... 16
cardinality      (-c) ... 65536
key size         (-k) ... 16
data size        (-d) ... 64
watermark        (-w) ... 1024
seq 4494718-4495743, 603.9 MB/s, 7.190 M messages/s, 449.273 K op/s
seq 9055893-9056917, 612.8 MB/s, 7.295 M messages/s, 455.993 K op/s
seq 13656719-13657168, 618.2 MB/s, 7.359 M messages/s, 459.988 K op/s
```

## 1024 clients, UNIX-domain socket

```sh
% ./bench_concurrent -r 1024
# of I/O threads (-t) ... 1
# of replicas    (-r) ... 1024
cardinality      (-c) ... 65536
key size         (-k) ... 16
data size        (-d) ... 64
watermark        (-w) ... 1024
seq 76672-77699, 659.8 MB/s, 7.855 M messages/s, 7.667 K op/s
seq 154624-155656, 670.5 MB/s, 7.982 M messages/s, 7.792 K op/s
seq 230805-231831, 654.7 MB/s, 7.795 M messages/s, 7.614 K op/s
```
