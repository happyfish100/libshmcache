.SUFFIXES: .c .o

COMPILE = $(CC) -g -O1 -Wall -D_FILE_OFFSET_BITS=64 -g -DDEBUG_FLAG
INC_PATH = -I/usr/include/fastcommon -I/usr/include/shmcache
LIB_PATH = -lfastcommon -lshmcache -lpthread -ldl
TARGET_PATH = $(DESTDIR)/usr/bin/

TARGET_PRGS = shmcache_set shmcache_get shmcache_delete shmcache_remove_all \
			  shmcache_stats

ALL_PRGS = $(TARGET_PRGS)

all: $(ALL_PRGS)
.c:
	$(COMPILE) -o $@ $< $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
install:
	mkdir -p $(TARGET_PATH)
	install -m 755 $(TARGET_PRGS) $(TARGET_PATH)
clean:
	rm -f $(ALL_PRGS)

