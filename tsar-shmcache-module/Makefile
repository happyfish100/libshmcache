CFLAGS = -Wall -fPIC --shared -g -O2
CC = gcc
INCLUDE_DIR = /usr/local/tsar/devel
LINK = $(CC) -I$(INCLUDE_DIR) -I/usr/include/shmcache  -I/usr/include/fastcommon \
	   -lfastcommon -lshmcache $(CFLAGS)

OBJS =  mod_shmcache.so

all: $(OBJS)

$(OBJS): %.so: %.c
	$(LINK) $< -o $@
clean:
	rm -f *.so;
install:
	mkdir -p $(PREFIX)/etc/tsar/conf.d/
	mkdir -p $(PREFIX)/usr/local/tsar/modules/
	cp ./mod_shmcache.so $(PREFIX)/usr/local/tsar/modules/
	cp ./mod_shmcache.conf $(PREFIX)/etc/tsar/conf.d/shmcache.conf
uninstall:
	rm $(PREFIX)/usr/local/tsar/modules/mod_shmcache.so
	rm $(PREFIX)/etc/tsar/conf.d/shmcache.conf
