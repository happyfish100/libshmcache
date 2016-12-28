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
  * check some key fields for consistency when init, the old share memories
    should be cleaned and reinit when the memory related parameters changed
  * sleep some time to avoid other processes read dirty data when
    recycle more than one valid (not expired) hash/KV entries

other features are:
  * allocate value buffer segment incrementally as need, controlled by
    this config parameter: segment_size
  * supply perfect stats info: counters for set, get and delete,
    memory recycle stats, lock stats etc.


libshmcache PHP extension in diretory: php-shmcache
PHP APIs
ShmCache::__construct(string config_filename[, long serializer =
        ShmCache::SERIALIZER_IGBINARY])
@param config_filename: the config filename as conf/libshmcache.conf
@param serializer: the serializer type
    ShmCache::SERIALIZER_IGBINARY for igbinary, the default serializer
    ShmCache::SERIALIZER_MSGPACK for msgpack
    ShmCache::SERIALIZER_PHP for php serializer
    ShmCache::SERIALIZER_NONE only support string variable
@example: $cache = new ShmCache("/etc/libshmcache.conf");
@note: igbinary and msgpack php extensions must be enabled before use them
    check method:
    php -m | grep igbinary
    php -m | grep msgpack

boolean ShmCache::set(string key, mixed value, long ttl)
@param key: the key, must be a string variable
@param value: the value, any php variable
@param ttl: timeout / expire in seconds, such as 600 for ten minutes
    ShmCache::NEVER_EXPIRED for never expired
@return true for success, false for fail
@example: $cache->set($key, $value, 300);

mixed ShmCache::get(string key)
@param key: the key, must be a string variable
@return mixed value for success, false for key not exist or expired
@example: $value = $cache->get($key);

boolean ShmCache::delete(string key)
@param key: the key, must be a string variable
@return true for success, false for fail
@example: $cache->delete($key);

array ShmCache::stats()
@return stats array
@example: echo json_encode($cache->stats(), JSON_PRETTY_PRINT);
