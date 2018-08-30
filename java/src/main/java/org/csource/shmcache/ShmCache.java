package org.csource.shmcache;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.io.UnsupportedEncodingException;
import java.io.StringReader;
import javax.json.Json;
import javax.json.JsonReader;
import javax.json.JsonArray;
import javax.json.JsonObject;
import javax.json.JsonValue;

public class ShmCache {
    public static class Value {
        private byte[] value;
        private int options;  //options for application
        private long expires; //expire time in millisenconds

        public Value(byte[] value, int options, long expires) {
            this.value = value;
            this.options = options;
            this.expires = expires;
        }

        public byte[] getValue() {
            return this.value;
        }

        public int getOptions() {
            return this.options;
        }

        public long getExpires() {
            return this.expires;
        }

        public String toString() {
            try {
                return "value: " + new String(this.value, ShmCache.charset)
                    + ", options: " + this.options + ", expires: " + this.expires;
            } catch (UnsupportedEncodingException ex) {
                throw new RuntimeException(ex);
            }
        }
    }

    public static final int TTL_NEVER_EXPIRED = 0;

    public static final int SHMCACHE_SERIALIZER_STRING   = 0x1;   //string type
    public static final int SHMCACHE_SERIALIZER_LIST     = (SHMCACHE_SERIALIZER_STRING | 0x2);
    public static final int SHMCACHE_SERIALIZER_MAP      = (SHMCACHE_SERIALIZER_STRING | 0x4);
    public static final int SHMCACHE_SERIALIZER_INTEGER  = (SHMCACHE_SERIALIZER_STRING | 0x8);
    public static final int SHMCACHE_SERIALIZER_IGBINARY = 0x200;
    public static final int SHMCACHE_SERIALIZER_MSGPACK  = 0x400;
    public static final int SHMCACHE_SERIALIZER_PHP      = 0x800;

    private static HashMap<String, ShmCache> instances = new HashMap<String, ShmCache>();
    private static String charset = "UTF-8";
    private static String libraryFilename = null;

    private long handler;

    private native long doInit(String configFilename);
    private native void doDestroy(long handler);
    private native void doSetVO(long handler, String key, Value value);
    private native void doSet(long handler, String key, byte[] value, int ttl);
    private native long doIncr(long handler, String key, long increment, int ttl);
    private native byte[] doGetBytes(long handler, String key);
    private native Value doGet(long handler, String key);
    private native boolean doDelete(long handler, String key);
    private native boolean doSetTTL(long handler, String key, int ttl);
    private native long doGetExpires(long handler, String key);
    private native boolean doClear(long handler);
    private native long doGetLastClearTime(long handler);
    private native long doGetInitTime(long handler);

    public static String getLibraryFilename() {
        return libraryFilename;
    }

    /**
      * set libshmcachejni.so filename to load
      * @param filename the full filename
      * @return none
      */
    public static void setLibraryFilename(String filename) {
        if (libraryFilename == null) {
            System.load(filename);  //load the library
            libraryFilename = filename;
        } else if (libraryFilename.equals(filename)) {
            System.err.println("[WARNING] library " + libraryFilename + " already loaded");
        } else {
            throw new RuntimeException("library " + libraryFilename
                    + " already loaded, can't change to " + filename);
        }
    }

    public static String getCharset() {
        return charset;
    }

    public static void setCharset(String value) {
        charset = value;
    }

    // private for singleton
    private ShmCache(String configFilename) {
        this.handler = doInit(configFilename);
    }

    protected void finalize() throws Throwable {
        super.finalize();
        this.close();
    }

    private void close() {
        System.out.println("close");
        doDestroy(this.handler);
    }

    /**
      * get ShmCache instance
      * @param configFilename the config filename such as /usr/local/etc/libshmcache.conf
      * @return ShmCache object
     */
    public synchronized static ShmCache getInstance(String configFilename) {
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
    public synchronized static void clearInstances() {
        instances.clear();
    }

    /**
      * get the value of the key
      * @param key the key
      * @return the value object, null for not exists
     */
    public Value getValue(String key) {
        return doGet(this.handler, key);
    }

    /**
      * get byte array value of the key
      * @param key the key
      * @return byte array value, null for not exists
     */
    public byte[] getBytes(String key) {
        return doGetBytes(this.handler, key);
    }

    /**
      * get string value of the key
      * @param key the key
      * @return string value, null for not exists
     */
    public String getString(String key) {
        byte[] value = doGetBytes(this.handler, key);
        try {
            return value != null ? new String(value, charset) : null;
        } catch (UnsupportedEncodingException ex) {
            throw new RuntimeException(ex);
        }
    }

    private String unquote(String sv)
    {
        if (sv.length() >= 2 && (sv.charAt(0) == '"' &&
                    sv.charAt(sv.length() - 1) == '"'))
        {
            return sv.substring(1, sv.length() - 1);
        } else {
            return sv;
        }
    }

    /**
      * json string to list
      * @param value the value
      * @return string list, null for not exists
     */
    public List<String> jsonToList(String value) {
        ArrayList<String> list = new ArrayList<String>();
        JsonReader jsonReader = Json.createReader(new StringReader(value));
        try {
            JsonArray array = jsonReader.readArray();
            for (JsonValue jvalue: array) {
                list.add(this.unquote(jvalue.toString()));
            }
        } finally {
            jsonReader.close();
        }

        return list;
    }

    /**
      * get string list of the key
      * @param key the key
      * @return string list, null for not exists
     */
    public List<String> getList(String key) {
        return jsonToList(getString(key));
    }

    /**
      * json string to map
      * @param value the value
      * @return string map, null for not exists
     */
    public Map<String, String> jsonToMap(String value) {
        if (value == null) {
            return null;
        }
        HashMap<String, String> map= new HashMap<String, String>();
        JsonReader jsonReader = Json.createReader(new StringReader(value));
        try {
            JsonObject jobject = jsonReader.readObject();
            for (Map.Entry<String, JsonValue> entry : jobject.entrySet()) {
                map.put(entry.getKey(), this.unquote(entry.getValue().toString()));
            }
        } finally {
            jsonReader.close();
        }

        return map;
    }

    /**
      * get string map of the key
      * @param key the key
      * @return string map, null for not exists
     */
    public Map<String, String> getMap(String key) {
        return jsonToMap(getString(key));
    }

    /**
      * get the value of the key
      * @param key the key
      * @return the value object (String, Long, List<String>, Map<String, String> or byte[]),
        null for not exists
     */
    public Object get(String key) {
        Value vo = doGet(this.handler, key);
        if (vo == null) {
            return null;
        }

        byte[] bs = vo.getValue();
        try {
            switch (vo.getOptions()) {
                case SHMCACHE_SERIALIZER_STRING:
                    return new String(bs, charset);
                case SHMCACHE_SERIALIZER_INTEGER:
                    return Long.parseLong(new String(bs, charset));
                case SHMCACHE_SERIALIZER_LIST:
                    return jsonToList(new String(bs, charset));
                case SHMCACHE_SERIALIZER_MAP:
                    return jsonToMap(new String(bs, charset));
                default:
                    return bs;
            }
        } catch (UnsupportedEncodingException ex) {
            throw new RuntimeException(ex);
        }
    }

    /**
      * set value object of the key
      * @param key the key
      * @param value the value object
      * @return none
     */
    public void set(String key, Value value) {
        doSetVO(this.handler, key, value);
    }

    /**
      * set value and TTL of the key
      * @param key the key
      * @param value the byte array value
      * @param ttl the TTL in seconds
      * @return none
     */
    public void set(String key, byte[] value, int ttl) {
        doSet(this.handler, key, value, ttl);
    }

    /**
      * set value and TTL of the key
      * @param key the key
      * @param value the string value
      * @param ttl the TTL in seconds
      * @return none
     */
    public void set(String key, String value, int ttl) {
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
    public boolean setTTL(String key, int ttl) {
        return doSetTTL(this.handler, key, ttl);
    }

    /**
      * increase value of the key
      * @param key the key
      * @param increment the increment such as 1
      * @param ttl the TTL in seconds
      * @return the value after increase
     */
    public long incr(String key, long increment, int ttl) {
        return doIncr(this.handler, key, increment, ttl);
    }

    /**
      * remove the key
      * @param key the key to remove
      * @return true for success, false for not exists
     */
    public boolean remove(String key) {
        return doDelete(this.handler, key);
    }

    /**
      * clear the shm, remove all keys
      * @return true for success, false for fail
     */
    public boolean clear() {
        return doClear(this.handler);
    }

    /**
      * get the expires timestamp of the key
      * @param key the key
      * @return expires timestamp in milliseconds
      *  return -1 when not exist
      *  return 0 for never expired
     */
    public long getExpires(String key) {
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
    public int getTTL(String key) {
        long expires = this.getExpires(key);
        return (int)(expires > 0 ? (expires - System.currentTimeMillis()) / 1000 : expires);
    }

    /**
      * get last clear timestamp
      * @return last clear timestamp in milliseconds
      *  return 0 for never cleared
     */
    public long getLastClearTime() {
        return doGetLastClearTime(this.handler) * 1000;
    }

    /**
      * get the shm init timestamp
      * @return the shm init timestamp in milliseconds
     */
    public long getInitTime() {
        return doGetInitTime(this.handler) * 1000;
    }

    public static void main(String[] args) throws UnsupportedEncodingException {
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

        System.out.println("value: " + shmCache.get(key));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));

        Value vo;
        vo = new Value(value.getBytes(charset), SHMCACHE_SERIALIZER_STRING,
                System.currentTimeMillis() + 300 * 1000);
        shmCache.set(key, vo);
        System.out.println(shmCache.get(key));

        shmCache.set(key, value, 300);
        System.out.println("value: " + shmCache.getString(key));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println(shmCache.getValue(key));

        System.out.println("set TTL: " + shmCache.setTTL(key, 600));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println(shmCache.getValue(key));

        System.out.println("after incr: " + shmCache.incr(key, 1, TTL_NEVER_EXPIRED));
        System.out.println("TTL: " + shmCache.getTTL(key) + ", expires: " + shmCache.getExpires(key));
        System.out.println(shmCache.getValue(key));

        System.out.println("remove key: " + shmCache.remove(key));


    }
}
