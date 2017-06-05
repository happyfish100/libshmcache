Copyright (C) 2016 Happy Fish / YuQing

libshmcache may be copied or modified under the terms of BSD License.

libshmcache is a local share memory cache for multi processes.
it is a high performance library because read mechanism is lockless.
libshmcache is 100+ times faster than a remote interface such as redis.

this project contains C library and a PHP extension.

Its high performance features include:
  * pthread mutex lock on write, read is lockless
  * use hash table for quickly setting, getting and deletion
  * use object pool (FIFO queue) for hash/KV entry allocation
  * use striping/block buffer allocator. in a memory striping,
    allocate value buffer in order. just only decrease allocator's
    used size when free the value buffer. recycle the whole memory
    striping/allocator when it's used size reach to zero after memory free up.
  * use FIFO elimination algorithm instead of LRU

stable features are:
  * deadlock detection and auto unlock that caused by other crushed process
  * check some key fields for consistency when init, old share memories
    should be cleaned and reinit when the memory related parameters changed
  * add sleep time to avoid other processes reading dirty data when
    recycle more than one valid (not expired) hash/KV entries

other features are:
  * support both related processes (such as parent process and subprocesses)
    and unrelated processes (such as php-fpm and php CLI, two php CLI processes, etc.)
  * incrementally allocate value buffer segment as need, this is controlled
    by config parameter: segment_size
  * provide abundant stats info: counters for set, get and delete,
    memory recycle stats, lock stats etc.
  * support atomic increment
  * PHP extension support multiple serializers: igbinary, msgpack, php and NONE.
    these serializers can coexist in a share memory.

utility commands in directory: src/tools, in /usr/bin/ after make && make install
  * shmcache_set: set a key
  * shmcache_get: get a key
  * shmcache_delete: delete a key
  * shmcache_remove_all: remove the share memory
  * shmcache_stats: show share memory statistics

  * Note: the key size can not be longer than 64 bytes

libshmcache PHP extension is supplied in the directory: php-shmcache, support PHP 5 and PHP 7

ShmCache::__construct(string $config_filename[, long $serializer =
        ShmCache::SERIALIZER_IGBINARY])
  * @param config_filename: the config filename as conf/libshmcache.conf
  * @param serializer: the serializer type
    <pre>
      ShmCache::SERIALIZER_IGBINARY for igbinary, the default serializer
      ShmCache::SERIALIZER_MSGPACK for msgpack
      ShmCache::SERIALIZER_PHP for php serializer
      ShmCache::SERIALIZER_NONE only support string variable
    </pre>
  * @throws ShmCacheException if the serializer not enabled
  * @example: $cache = new ShmCache("/etc/libshmcache.conf");
  * @note: igbinary and msgpack php extensions must be enabled before use them
    <pre>
    check method:
    php -m | grep igbinary
    php -m | grep msgpack
    </pre>

boolean ShmCache::set(string $key, mixed $value, long $ttl)
  * @param key: the key, must be a string variable
  * @param value: the value, any php variable
  * @param ttl: timeout / expire in seconds, such as 600 for ten minutes,
    ShmCache::NEVER_EXPIRED for never expired
  * @return true for success, false for fail
  * @throws ShmCacheException if $value is false
  * @example: $cache->set($key, $value, 300);

long ShmCache::incr(string $key, long $increment, long $ttl)
  * @desc: atomic increment
  * @param key: the key, must be a string variable
  * @param increment: the increment integer, can be negative, such as -1
  * @param ttl: timeout / expire in seconds, such as 600 for ten minutes,
  * @return the value after increase, false for fail

mixed ShmCache::get(string $key[, boolean $returnExpired = false])
  * @param key: the key, must be a string variable
  * @param returnExpired: if return expired key / value
  * @return mixed value for success, false for key not exist or expired
  * @example: $value = $cache->get($key);

long ShmCache::getExpires(string $key[, boolean $returnExpired = false])
  * @desc: get expires time as unix timestamp
  * @param key: the key, must be a string variable
  * @param returnExpired: if return expired key / value
  * @return expires timestamps such as 1483952635, 0 for never expired, false for not exist
  * @example: $value = $cache->getExpires($key);

boolean ShmCache::setExpires(string $key, long $expires)
  * @param key: the key, must be a string variable
  * @param expires: expires timestamp (unix timestamp eg. 1591347245)
    ShmCache::NEVER_EXPIRED for never expired
  * @return true for success, false for key not exist or expired or other error
  * @throws ShmCacheException if $expires is invalid
  * @example: $cache->setExpires($key, 1591347245);

boolean ShmCache::setTTL(string $key, long $ttl)
  * @param key: the key, must be a string variable
  * @param ttl: timeout / expire in seconds, such as 600 for ten minutes,
    ShmCache::NEVER_EXPIRED for never expired
  * @return true for success, false for key not exist or expired or other error
  * @throws ShmCacheException if $ttl is invalid
  * @example: $cache->setTTL($key, 300);

boolean ShmCache::delete(string $key)
  * @param key: the key, must be a string variable
  * @return true for success, false for fail
  * @example: $cache->delete($key);

array ShmCache::stats()
  * @return stats array
  * @example: echo json_encode($cache->stats(), JSON_PRETTY_PRINT);

boolean ShmCache::clear()
  * @desc: clear hashtable to empty. use this function carefully because it will remove all keys in cache
  * @return true for success, false for fail
