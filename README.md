Copyright (C) 2016 Happy Fish / YuQing

libshmcache may be copied only under the terms of the Less GNU General
Public License(LGPL).

libshmcache is a local share memory cache for multi processes.
it is high performance because read is lockless.

this project contains C library and PHP extension.

the high performance features are:
  * pthread mutext lock on write, read is lockless
  * use hash table for quickly set, get and delete
  * use object pool (FIFO queue) for hash/KV entry allocation
  * use striping/block buffer allocator. in a memory striping,
    allocate the value buffer in order. just only decrease the allocator's
    used size when free the value buffer. recycle the whole memory
    striping/allocator when it's used size is zero after free.
  * use FIFO elimination algorithm instead of LRU

the stable features are:
  * deadlock detect and auto unlock which caused by other crushed process
  * sleep some time to avoid other processes read dirty data when
    recycle more than one valid (not expired) hash/KV entries

other features are:
  * allocate value buffer segement incrementally as need, controled by
    this config parameter: segment_size
  * supply perfect stats info: counters for set, get and delete,
    memory recycle stats, lock stats etc.

