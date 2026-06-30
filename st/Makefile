# st - simple terminal
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = st.c x.c hb.c sixel.c sixel_hls.c
OBJ = $(SRC:.c=.o)

VERBOSE := 0
PRINTF := @printf "%-8s %s\n"
ifeq ($(VERBOSE), 0)
	Q := @
endif

all: st
	@size=$$(du -h st 2>/dev/null | cut -f1); \
	printf "%-8s st (%s)\n" "DONE" "$$size"

config.h:
	$(PRINTF) "GEN" $@
	$Qcp config.def.h config.h

.c.o:
	$(PRINTF) "CC" $@
	$Q$(CC) $(STCFLAGS) -c -o $@ $<

st.o: config.h st.h win.h
x.o: arg.h config.h st.h win.h hb.h
hb.o: st.h
sixel.o: config.h st.h win.h sixel.h sixel_hls.h
sixel_hls.o: sixel_hls.h

$(OBJ): config.h config.mk

st: $(OBJ)
	$(PRINTF) "LD" $@
	$Q$(CC) -o $@ $(OBJ) $(STLDFLAGS)

clean:
	$(PRINTF) "CLEAN" "binary + objects"
	$Qrm -f st $(OBJ) st-$(VERSION).tar.gz

dist: clean
	$(PRINTF) "DIST" st-$(VERSION).tar.gz
	$Qmkdir -p st-$(VERSION)
	$Qcp -R FAQ LEGACY TODO LICENSE Makefile README config.mk\
		config.def.h st.info st.1 arg.h st.h win.h $(SRC)\
		st-$(VERSION)
	$Qtar -cf - st-$(VERSION) | gzip > st-$(VERSION).tar.gz
	$Qrm -rf st-$(VERSION)

install: st
	$(PRINTF) "INSTALL" "$(DESTDIR)$(PREFIX)/bin/st"
	$Qmkdir -p $(DESTDIR)$(PREFIX)/bin
	$Qcp -f st $(DESTDIR)$(PREFIX)/bin
	$Qchmod 755 $(DESTDIR)$(PREFIX)/bin/st
	$(PRINTF) "MAN" "$(DESTDIR)$(MANPREFIX)/man1/st.1"
	$Qmkdir -p $(DESTDIR)$(MANPREFIX)/man1
	$Qsed "s/VERSION/$(VERSION)/g" < st.1 > $(DESTDIR)$(MANPREFIX)/man1/st.1
	$Qchmod 644 $(DESTDIR)$(MANPREFIX)/man1/st.1
	$(PRINTF) "TERMINFO" "St entry (tic -sx st.info)"
	$Qtic -sx st.info
	@echo "         See README for terminfo notes (TERM=st-256color, etc.)"

uninstall:
	$(PRINTF) "RM" "st + man page"
	$Qrm -f $(DESTDIR)$(PREFIX)/bin/st
	$Qrm -f $(DESTDIR)$(MANPREFIX)/man1/st.1

.PHONY: all clean dist install uninstall
