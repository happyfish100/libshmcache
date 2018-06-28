package org.csource.shmcache;

import java.util.Properties;
import java.io.IOException;
import org.springframework.beans.factory.config.PropertyPlaceholderConfigurer;

public class ShmCachePropertyConfigurer extends PropertyPlaceholderConfigurer {

    protected String libraryFilename;
    protected String configFilename;
    protected String charset = "UTF-8";
    protected boolean debug = false;
    protected ShmCache shmCache;

    public void setLibraryFilename(String libraryFilename) {
        this.libraryFilename = libraryFilename;
    }

    public void setConfigFilename(String configFilename) {
        this.configFilename = configFilename;
    }

    public void setCharset(String charset) {
        this.charset = charset;
    }

    public void setDebug(String value) {
        this.debug = value.equals("1") || value.equals("true");
    }

    protected void initShmCache() {
        if (this.shmCache != null) {
            return;
        }

        if (this.charset != null) {
            ShmCache.setCharset(this.charset);
        }
        ShmCache.setLibraryFilename(this.libraryFilename);
        this.shmCache = ShmCache.getInstance(this.configFilename);
    }

    @Override
    protected void loadProperties(Properties props) throws IOException {
        super.loadProperties(props);

        this.initShmCache();
        ShmCachePropertyConfigAnnotation.setDebug(this.debug);
        ShmCachePropertyConfigAnnotation.processProperties(shmCache, props);
    }

    public String getConfig(String key) {
        this.initShmCache();
        return this.shmCache.getString(key);
    }
}
