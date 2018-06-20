.SUFFIXES: .c .o .lo

COMPILE = $(CC) -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -O3
INC_PATH = -I/usr/local/include -I/Library/Java/JavaVirtualMachines/jdk1.8.0_162.jdk/Contents/Home/include \
           -I/Library/Java/JavaVirtualMachines/jdk1.8.0_151.jdk/Contents/Home/include/darwin
LIB_PATH = -lfastcommon -lshmcache
PREFIX=/usr/local

SHMCACHE_SHARED_OBJS = org_csource_shmcache_ShmCache.lo

ALL_OBJS = $(SHMCACHE_SHARED_OBJS)

ALL_PRGS =
SHARED_LIBS = libshmcachejni.so
ALL_LIBS = $(SHARED_LIBS)

all: $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)
libshmcachejni.so:
	$(COMPILE) -o $@ $< -shared $(SHMCACHE_SHARED_OBJS) $(LIB_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
.c.lo:
	$(COMPILE) -c -fPIC -o $@ $<  $(INC_PATH)
install:
	mkdir -p $(PREFIX)/lib64
	mkdir -p $(PREFIX)/lib
	install -m 755 $(SHARED_LIBS) $(PREFIX)/lib64
	install -m 755 $(SHARED_LIBS) $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/shmcache
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)

