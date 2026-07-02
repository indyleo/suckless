
# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c ipc.c movestack.c screenshot.c util.c wallpaper.c
OBJ = ${SRC:.c=.o}

VERBOSE := 0
PRINTF := @printf "%-8s %s\n"
ifeq ($(VERBOSE), 0)
	Q := @
endif

all: dwm
	@size=$$(du -h dwm 2>/dev/null | cut -f1); \
	printf "%-8s dwm (%s)\n" "DONE" "$$size"

.c.o:
	$(PRINTF) "CC" $@
	$Q${CC} -c ${CFLAGS} -o $@ $<

${OBJ}: config.h config.mk

config.h:
	$(PRINTF) "GEN" $@
	$Qcp config.def.h $@

dwm: ${OBJ}
	$(PRINTF) "LD" $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	$(PRINTF) "CLEAN" "binary + objects"
	$Qrm -f dwm ${OBJ} dwm-${VERSION}.tar.gz

dist: clean
	$(PRINTF) "DIST" dwm-${VERSION}.tar.gz
	$Qmkdir -p dwm-${VERSION}
	$Qcp -R LICENSE Makefile README config.def.h config.mk\
		dwm.1 drw.h dwm.h ipc.h movestack.h screenshot.h wallpaper.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	$Qtar -cf dwm-${VERSION}.tar dwm-${VERSION}
	$Qgzip dwm-${VERSION}.tar
	$Qrm -rf dwm-${VERSION}

install: all
	$(PRINTF) "INSTALL" "${DESTDIR}${PREFIX}/bin/dwm"
	$Qmkdir -p ${DESTDIR}${PREFIX}/bin
	$Qcp -f dwm ${DESTDIR}${PREFIX}/bin
	$Qchmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	$(PRINTF) "MAN" "${DESTDIR}${MANPREFIX}/man1/dwm.1"
	$Qmkdir -p ${DESTDIR}${MANPREFIX}/man1
	$Qsed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	$Qchmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1
	$(PRINTF) "INSTALL" "${DESTDIR}${PREFIX}/bin/autostart.sh"
	$Qmkdir -p ${DESTDIR}${PREFIX}/bin
	$Qcp -f autostart.sh ${DESTDIR}${PREFIX}/bin/autostart.sh
	$Qchmod 755 ${DESTDIR}${PREFIX}/bin/autostart.sh
	$(PRINTF) "ICON" "${DESTDIR}/usr/share/icons/hicolor/48x48/apps/dwm.png"
	$Qmkdir -p ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/
	$Qcp -f dwm.png ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/dwm.png
	$(PRINTF) "DESKTOP" "${DESTDIR}/usr/share/xsessions/dwm.desktop"
	$Qmkdir -p ${DESTDIR}/usr/share/xsessions/
	$Qcp -f dwm.desktop ${DESTDIR}/usr/share/xsessions/dwm.desktop

uninstall:
	$(PRINTF) "RM" "dwm + autostart.sh + icon + desktop entry + man page"
	$Qrm -f ${DESTDIR}${PREFIX}/bin/dwm \
		${DESTDIR}${PREFIX}/bin/autostart.sh \
		${DESTDIR}/usr/share/icons/hicolor/48x48/apps/dwm.png \
		${DESTDIR}/usr/share/xsessions/dwm.desktop \
		${DESTDIR}${MANPREFIX}/man1/dwm.1

.PHONY: all clean dist install uninstall
