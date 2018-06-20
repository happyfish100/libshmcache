package org.csource.shmcache;

import java.util.HashMap;

public class ShmCache {

    public static final int TTL_NEVER_EXPIRED = 0;

    private static HashMap<String, ShmCache> instances = new HashMap<String, ShmCache>();
    private static String charset = "UTF-8";
    private long handler;

    private native long doInit(String configFilename);
    private native void doDestroy(long handler);
    private native void doSet(long handler, String key, byte[] value, int ttl);
    private native long doIncr(long handler, String key, long increment, int ttl);
    private native byte[] doGet(long handler, String key);
    private native boolean doDelete(long handler, String key);
    private native boolean doSetTTL(long handler, String key, int ttl);
    private native long doGetExpires(long handler, String key);
    private native boolean doRemoveAll(long handler);
    private native boolean doClear(long handler);

    static{
        System.load("/Users/yuqing/Devel/libshmcache/java/jni/libshmcachejni.so");
    }

    public static String getCharset()
    {
        return charset;
    }

    public static void setCharset(String value)
    {
        charset = value;
    }

    private ShmCache(String configFilename)
    {
        this.handler = doInit(configFilename);
    }

    private void close()
    {
        doDestroy(this.handler);
    }

    public synchronized static ShmCache getInstance(String configFilename)
    {
        ShmCache obj = instances.get(configFilename);
        if (obj == null) {
            obj = new ShmCache(configFilename);
            instances.put(configFilename, obj);
        }

        return obj;
    }

    public byte[] get(String key)
    {
        return doGet(this.handler, key);
    }

    public String getString(String key) throws Exception
    {
        byte[] value = doGet(this.handler, key);
        return value != null ? new String(value, charset) : null;
    }

    public void set(String key, byte[] value, int ttl)
    {
        doSet(this.handler, key, value, ttl);
    }

    public void set(String key, String value, int ttl) throws Exception
    {
        doSet(this.handler, key, value.getBytes(charset), ttl);
    }

    public boolean setTTL(String key, int ttl)
    {
        return doSetTTL(this.handler, key, ttl);
    }

    public long incr(String key, long increment, int ttl)
    {
        return doIncr(this.handler, key, increment, ttl);
    }

    public long getExpires(String key)
    {
        long expires = doGetExpires(this.handler, key);
        return expires > 0 ? (expires * 1000) : expires;
    }

    public int getTTL(String key)
    {
        long expires = this.getExpires(key);
        return (int)(expires > 0 ? (expires - System.currentTimeMillis()) / 1000 : expires);
    }

    public boolean remove(String key)
    {
        return doDelete(this.handler, key);
    }

    public boolean removeAll()
    {
        return doRemoveAll(this.handler);
    }

    public boolean clear()
    {
        return doClear(this.handler);
    }

    public static void main(String[] args) throws Exception
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

        ShmCache shmCache = ShmCache.getInstance(configFilename);
        shmCache.set(key, value, 300);
        System.out.println(shmCache.getString(key));
        System.out.println(shmCache.incr(key, 1, TTL_NEVER_EXPIRED));
        System.out.println(shmCache.remove(key));
    }
}
