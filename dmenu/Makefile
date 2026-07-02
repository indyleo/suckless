# dmenu - dynamic menu
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dmenu.c stest.c util.c
OBJ = $(SRC:.c=.o)

VERBOSE := 0
PRINTF := @printf "%-8s %s\n"
ifeq ($(VERBOSE), 0)
	Q := @
endif

all: dmenu stest
	@size1=$$(du -h dmenu  2>/dev/null | cut -f1); \
	size2=$$(du -h stest  2>/dev/null | cut -f1); \
	printf "%-8s dmenu (%s), stest (%s)\n" "DONE" "$$size1" "$$size2"

.c.o:
	$(PRINTF) "CC" $@
	$Q$(CC) -c $(CFLAGS) -o $@ $<

config.h:
	$(PRINTF) "GEN" $@
	$Qcp config.def.h $@

$(OBJ): arg.h config.h config.mk drw.h

dmenu: dmenu.o drw.o util.o
	$(PRINTF) "LD" $@
	$Q$(CC) -o $@ dmenu.o drw.o util.o $(LDFLAGS)

stest: stest.o
	$(PRINTF) "LD" $@
	$Q$(CC) -o $@ stest.o $(LDFLAGS)

clean:
	$(PRINTF) "CLEAN" "binaries + objects"
	$Qrm -f dmenu stest $(OBJ) dmenu-$(VERSION).tar.gz

dist: clean
	$(PRINTF) "DIST" dmenu-$(VERSION).tar.gz
	$Qmkdir -p dmenu-$(VERSION)
	$Qcp LICENSE Makefile README arg.h config.def.h config.mk dmenu.1\
		drw.h util.h dmenu_path dmenu_run stest.1 $(SRC)\
		dmenu-$(VERSION)
	$Qtar -cf dmenu-$(VERSION).tar dmenu-$(VERSION)
	$Qgzip dmenu-$(VERSION).tar
	$Qrm -rf dmenu-$(VERSION)

install: all
	$(PRINTF) "INSTALL" "$(DESTDIR)$(PREFIX)/bin/{dmenu,dmenu_path,dmenu_run,stest}"
	$Qmkdir -p $(DESTDIR)$(PREFIX)/bin
	$Qcp -f dmenu dmenu_path dmenu_run stest $(DESTDIR)$(PREFIX)/bin
	$Qchmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu
	$Qchmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu_path
	$Qchmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu_run
	$Qchmod 755 $(DESTDIR)$(PREFIX)/bin/stest
	$(PRINTF) "MAN" "$(DESTDIR)$(MANPREFIX)/man1/{dmenu,stest}.1"
	$Qmkdir -p $(DESTDIR)$(MANPREFIX)/man1
	$Qsed "s/VERSION/$(VERSION)/g" < dmenu.1 > $(DESTDIR)$(MANPREFIX)/man1/dmenu.1
	$Qsed "s/VERSION/$(VERSION)/g" < stest.1 > $(DESTDIR)$(MANPREFIX)/man1/stest.1
	$Qchmod 644 $(DESTDIR)$(MANPREFIX)/man1/dmenu.1
	$Qchmod 644 $(DESTDIR)$(MANPREFIX)/man1/stest.1

uninstall:
	$(PRINTF) "RM" "dmenu, dmenu_path, dmenu_run, stest + man pages"
	$Qrm -f $(DESTDIR)$(PREFIX)/bin/dmenu\
		$(DESTDIR)$(PREFIX)/bin/dmenu_path\
		$(DESTDIR)$(PREFIX)/bin/dmenu_run\
		$(DESTDIR)$(PREFIX)/bin/stest\
		$(DESTDIR)$(MANPREFIX)/man1/dmenu.1\
		$(DESTDIR)$(MANPREFIX)/man1/stest.1

.PHONY: all clean dist install uninstall
