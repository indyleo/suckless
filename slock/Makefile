# slock - simple screen locker
# See LICENSE file for copyright and license details.

include config.mk

SRC = slock.c ${COMPATSRC}
OBJ = ${SRC:.c=.o}

VERBOSE := 0
PRINTF := @printf "%-8s %s\n"
ifeq ($(VERBOSE), 0)
	Q := @
endif

all: slock
	@size=$$(du -h slock 2>/dev/null | cut -f1); \
	printf "%-8s slock (%s)\n" "DONE" "$$size"

.c.o:
	$(PRINTF) "CC" $@
	$Q${CC} -c ${CFLAGS} -o $@ $<

${OBJ}: config.h config.mk arg.h util.h

config.h:
	$(PRINTF) "GEN" $@
	$Qcp config.def.h $@

slock: ${OBJ}
	$(PRINTF) "LD" $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	$(PRINTF) "CLEAN" "binary + objects"
	$Qrm -f slock ${OBJ} slock-${VERSION}.tar.gz

dist: clean
	$(PRINTF) "DIST" slock-${VERSION}.tar.gz
	$Qmkdir -p slock-${VERSION}
	$Qcp -R LICENSE Makefile README slock.1 config.mk \
		${SRC} config.def.h arg.h util.h slock-${VERSION}
	$Qtar -cf slock-${VERSION}.tar slock-${VERSION}
	$Qgzip slock-${VERSION}.tar
	$Qrm -rf slock-${VERSION}

install: all
	$(PRINTF) "INSTALL" "${DESTDIR}${PREFIX}/bin/slock"
	$Qmkdir -p ${DESTDIR}${PREFIX}/bin
	$Qcp -f slock ${DESTDIR}${PREFIX}/bin
	$Qchmod 755 ${DESTDIR}${PREFIX}/bin/slock
	$(PRINTF) "SETUID" "${DESTDIR}${PREFIX}/bin/slock"
	$Qchmod u+s ${DESTDIR}${PREFIX}/bin/slock
	$(PRINTF) "MAN" "${DESTDIR}${MANPREFIX}/man1/slock.1"
	$Qmkdir -p ${DESTDIR}${MANPREFIX}/man1
	$Qsed "s/VERSION/${VERSION}/g" <slock.1 >${DESTDIR}${MANPREFIX}/man1/slock.1
	$Qchmod 644 ${DESTDIR}${MANPREFIX}/man1/slock.1

uninstall:
	$(PRINTF) "RM" "slock + man page"
	$Qrm -f ${DESTDIR}${PREFIX}/bin/slock
	$Qrm -f ${DESTDIR}${MANPREFIX}/man1/slock.1

.PHONY: all clean dist install uninstall
