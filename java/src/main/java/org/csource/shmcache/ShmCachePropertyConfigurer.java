package org.csource.shmcache;

import java.util.Properties;
import java.io.IOException;

import org.springframework.beans.factory.config.PropertyPlaceholderConfigurer;

public class ShmCachePropertyConfigurer extends PropertyPlaceholderConfigurer {

    protected String libraryFilename;
    protected String configFilename;
    protected String charset = "UTF-8";
    protected boolean debug = false;

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

    @Override
    protected void loadProperties(Properties props) throws IOException {
        super.loadProperties(props);

        if (this.charset != null) {
            ShmCache.setCharset(this.charset);
        }
        ShmCache.setLibraryFilename(this.libraryFilename);
        ShmCache shmCache = ShmCache.getInstance(this.configFilename);

        ShmCachePropertyConfigAnnotation.setDebug(this.debug);
        ShmCachePropertyConfigAnnotation.processProperties(shmCache, props);
    }
}
