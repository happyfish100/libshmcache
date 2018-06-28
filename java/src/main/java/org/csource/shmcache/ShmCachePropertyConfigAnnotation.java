package org.csource.shmcache;

import java.util.Properties;
import java.util.ArrayList;
import java.util.Map;
import java.util.Set;

public class ShmCachePropertyConfigAnnotation {

    public static final String CONFIG_TAG = "@config";
    private static boolean debug = false;

    private static class KeyValue {
        private String key;
        private String value;

        public KeyValue(String key, String value) {
            this.key = key;
            this.value = value;
        }

        public String getKey() {
            return this.key;
        }

        public String getValue() {
            return this.value;
        }
    }

    private ShmCachePropertyConfigAnnotation() {
    }

    public static void setDebug(boolean value) {
        debug = value;
    }

    protected static String processProperties(ShmCache shmCache, String key, String value) {
        value = value.substring(CONFIG_TAG.length()).trim();
        if (!(value.startsWith("(") && value.endsWith(")"))) {
            return null;
        }

        String params = value.substring(1, value.length() - 1);
        String[] parts = params.split(",");
        if (parts.length > 2) {
            System.err.println("too many parameters!");
            return null;
        }

        String keyParam = parts[0].trim();
        String defaultValue = parts.length == 2 ? parts[1].trim() : "";
        if (keyParam.length() == 0) {
            keyParam = key;
        }
        if (debug) {
            System.err.println("key: " + keyParam + ", default value: " + defaultValue);
        }

        String v = shmCache.getString(keyParam);
        if (v == null) {
            System.err.println("key: " + keyParam + " not found, set to default: " + defaultValue);
            v = defaultValue;
        }

        return v;
    }

    public static void processProperties(ShmCache shmCache, Properties props) {

        ArrayList<KeyValue> modifiedKV = new ArrayList<KeyValue>();
        Set<Map.Entry<Object, Object>> entries = props.entrySet();
        for (Map.Entry entry : entries) {
            String key = (String)entry.getKey();
            String value = (String)entry.getValue();

            if (value.startsWith(CONFIG_TAG)) {
                if (debug) {
                    System.err.println("processing: " + key + "=" + value);
                }
                String newValue = processProperties(shmCache, key, value);
                if (newValue != null) {
                    modifiedKV.add(new KeyValue(key, newValue));
                }
            }
        }

        for (KeyValue kv : modifiedKV) {
            props.put(kv.getKey(), kv.getValue());
            if (debug) {
                System.err.println("set key: " + kv.getKey() + " to value: " + kv.getValue());
            }
        }
        if (debug) {
            System.err.println("@config key count: " + modifiedKV.size());
        }
    }
}
