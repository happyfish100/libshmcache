.SUFFIXES: .c .o .lo

COMPILE = $(CC) -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -O3
INC_PATH = -I/usr/include/fastcommon
LIB_PATH = -lm -lpthread -lfastcommon

SHMCACHE_SHARED_OBJS = shmcache.lo shmopt.lo shm_striping_allocator.lo shm_object_pool.lo \
					   shm_hashtable.lo shm_value_allocator.lo shm_op_wrapper.lo shm_lock.lo

SHMCACHE_STATIC_OBJS = shmcache.o shmopt.o shm_striping_allocator.o shm_object_pool.o \
					   shm_hashtable.o shm_value_allocator.o shm_op_wrapper.o shm_lock.o

HEADER_FILES = shmcache.h shmcache_types.h shm_list.h shm_striping_allocator.h \
			   shm_value_allocator.h shm_op_wrapper.h shmopt.h shm_hashtable.h

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

