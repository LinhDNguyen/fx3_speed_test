CC = gcc -pipe
VERSION = 1.0
CFLAGS = -O2 -fno-exceptions -DUSBTESTAPP_VERSION=\"$(VERSION)\" \
	-W -Wall -Wformat
LDFLAGS = -lusb-1.0 -lpthread
DIST_DEST = BulkLoopApp-$(VERSION)

all: BulkLoopApp

# NOTE: Also add sources to the "dist:" target!
BulkLoopApp: bulkloopapp.o
	$(CC) bulkloopapp.o $(LDFLAGS) -o BulkLoopApp

clean:
	-rm -f bulkloopapp.o BulkLoopApp

distclean: clean
	-rm -f BulkLoopApp

dist:
	mkdir -p "$(DIST_DEST)"
	cp Makefile "$(DIST_DEST)"
	cp bulkloopapp.h "$(DIST_DEST)"
	cp bulkloopapp.c "$(DIST_DEST)"
	tar -c "$(DIST_DEST)" | gzip -9 > "BulkLoopApp-$(VERSION).tar.gz"
	rm -r "$(DIST_DEST)"

.cc.o:
	$(CC) -c $(CFLAGS) $<

bulkloopapp.o: bulkloopapp.c bulkloopapp.h
