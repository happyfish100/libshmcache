.SUFFIXES: .c .o .lo

COMPILE = $(CC) -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -O3
INC_PATH = 
LIB_PATH = -lm -lpthread

SHMCACHE_SHARED_OBJS = 

SHMCACHE_STATIC_OBJS = 

HEADER_FILES = 

ALL_OBJS = $(SHMCACHE_STATIC_OBJS) $(SHMCACHE_SHARED_OBJS)

ALL_PRGS = 
SHARED_LIBS = libshmcache.so
STATIC_LIBS = libshmcache.a
ALL_LIBS = $(SHARED_LIBS) $(STATIC_LIBS)

all: $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)
libshmcache.so:
	$(COMPILE) -o $@ $< -shared $(SHMCACHE_SHARED_OBJS) $(LIB_PATH)
libshmcache.a: $(SHMCACHE_STATIC_OBJS)
	ar rcs $@ $(SHMCACHE_STATIC_OBJS)
.o:
	$(COMPILE) -o $@ $<  $(SHMCACHE_STATIC_OBJS) $(LIB_PATH) $(INC_PATH)
.c:
	$(COMPILE) -o $@ $<  $(SHMCACHE_STATIC_OBJS) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
.c.lo:
	$(COMPILE) -c -fPIC -o $@ $<  $(INC_PATH)
install:
	mkdir -p $(DESTDIR)/usr/lib64
	mkdir -p $(DESTDIR)/usr/lib
	install -m 755 $(SHARED_LIBS) $(DESTDIR)/usr/lib64
	install -m 755 $(SHARED_LIBS) $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include/shmcache
	install -m 644 $(HEADER_FILES) $(DESTDIR)/usr/include/shmcache
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)

