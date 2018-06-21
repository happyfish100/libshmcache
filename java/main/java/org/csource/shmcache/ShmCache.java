package org.csource.shmcache;

import java.util.HashMap;
import java.io.UnsupportedEncodingException;

public class ShmCache {

    public static final int TTL_NEVER_EXPIRED = 0;

    private static HashMap<String, ShmCache> instances = new HashMap<String, ShmCache>();
    private static String charset = "UTF-8";
    private static String libraryFilename = null;

    private long handler;

    private native long doInit(String configFilename);
    private native void doDestroy(long handler);
    private native void doSet(long handler, String key, byte[] value, int ttl);
    private native long doIncr(long handler, String key, long increment, int ttl);
    private native byte[] doGet(long handler, String key);
    private native boolean doDelete(long handler, String key);
    private native boolean doSetTTL(long handler, String key, int ttl);
    private native long doGetExpires(long handler, String key);
    private native boolean doClear(long handler);
    private native long doGetLastClearTime(long handler);
    private native long doGetInitTime(long handler);

    public static String getLibraryFilename()
    {
        return libraryFilename;
    }

    public static void setLibraryFilename(String value)
    {
        if (libraryFilename == null) {
            System.load(value);  //load the library
            libraryFilename = value;
        } else {
            throw new RuntimeException("library " + libraryFilename + " already loaded");
        }
    }

    public static String getCharset()
    {
        return charset;
    }

    public static void setCharset(String value)
    {
        charset = value;
    }

    // private for singleton
    private ShmCache(String configFilename)
    {
        this.handler = doInit(configFilename);
    }

    protected void finalize() throws Throwable
    {
        super.finalize();
        this.close();
    }

    private void close()
    {
        System.out.println("close");
        doDestroy(this.handler);
    }

    /**
      * get ShmCache instance
      * @param configFilename the config filename such as /usr/local/etc/libshmcache.conf
      * @return ShmCache object
     */
    public synchronized static ShmCache getInstance(String configFilename)
    {
        ShmCache obj = instances.get(configFilename);
        if (obj == null) {
            obj = new ShmCache(configFilename);
            instances.put(configFilename, obj);
        }

        return obj;
    }

    /**
      * clear ShmCache instances
      * @return none
     */
    public synchronized static void clearInstances()
    {
        instances.clear();
    }

    /**
      * get byte array value of the key
      * @param key the key
      * @return byte array value, null for not exists
     */
    public byte[] get(String key)
    {
        return doGet(this.handler, key);
    }

    /**
      * get string value of the key
      * @param key the key
      * @return string value, null for not exists
     */
    public String getString(String key)
    {
        byte[] value = doGet(this.handler, key);
        try {
            return value != null ? new String(value, charset) : null;
        } catch (UnsupportedEncodingException ex) {
            throw new RuntimeException(ex);
        }
    }

    /**
      * set value and TTL of the key
      * @param key the key
      * @param value the byte array value
      * @param ttl the TTL in seconds
      * @return none
     */
    public void set(String key, byte[] value, int ttl)
    {
        doSet(this.handler, key, value, ttl);
    }

    /**
      * set value and TTL of the key
      * @param key the key
      * @param value the string value
      * @param ttl the TTL in seconds
      * @return none
     */
    public void set(String key, String value, int ttl)
    {
        try {
            doSet(this.handler, key, value.getBytes(charset), ttl);
        } catch (UnsupportedEncodingException ex) {
            throw new RuntimeException(ex);
        }
    }

    /**
      * set TTL of the key
      * @param key the key
      * @param ttl the TTL in seconds
      * @return true for success, otherwise fail
     */
    public boolean setTTL(String key, int ttl)
    {
        return doSetTTL(this.handler, key, ttl);
    }

    /**
      * increase value of the key
      * @param key the key
      * @param increment the increment such as 1
      * @param ttl the TTL in seconds
      * @return the value after increase
     */
    public long incr(String key, long increment, int ttl)
    {
        return doIncr(this.handler, key, increment, ttl);
    }

    /**
      * remove the key
      * @param key the key to remove
      * @return true for success, false for not exists
     */
    public boolean remove(String key)
    {
        return doDelete(this.handler, key);
    }

    /**
      * clear the shm, remove all keys
      * @return true for success, false for fail
     */
    public boolean clear()
    {
        return doClear(this.handler);
    }

    /**
      * get the expires timestamp of the key
      * @param key the key
      * @return expires timestamp in milliseconds
      *  return -1 when not exist
      *  return 0 for never expired
     */
    public long getExpires(String key)
    {
        long expires = doGetExpires(this.handler, key);
        return expires > 0 ? (expires * 1000) : expires;
    }

    /**
      * get the TTL of the key
      * @param key the key
      * @return the ttl in seconds
      *  return -1 when not exist
      *  return 0 for never expired
     */
    public int getTTL(String key)
    {
        long expires = this.getExpires(key);
        return (int)(expires > 0 ? (expires - System.currentTimeMillis()) / 1000 : expires);
    }

    /**
      * get last clear timestamp
      * @return last clear timestamp in milliseconds
      *  return 0 for never cleared
     */
    public long getLastClearTime()
    {
        return doGetLastClearTime(this.handler) * 1000;
    }

    /**
      * get the shm init timestamp
      * @return the shm init timestamp in milliseconds
     */
    public long getInitTime()
    {
        return doGetInitTime(this.handler) * 1000;
    }

    public static void main(String[] args)
    {
        String configFilename = "/usr/local/etc/libshmcache.conf";
        String key;
        String value;
        if (args.length > 0) {
            key = args[0];
        } else {
            key = "测试key";
        }
        if (args.length > 1) {
            value = args[1];
        } else {
            value = "测试value";
        }

        ShmCache.setLibraryFilename("/Users/yuqing/Devel/libshmcache/java/jni/libshmcachejni.so");
        ShmCache shmCache = ShmCache.getInstance(configFilename);
        shmCache.set(key, value, 300);
        System.out.println("value: " + shmCache.getString(key));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println("set TTL: " + shmCache.setTTL(key, 600));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println("after incr: " + shmCache.incr(key, 1, TTL_NEVER_EXPIRED));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println("remove key: " + shmCache.remove(key));
    }
}
