# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c util.c
OBJ = ${SRC:.c=.o}

all: dwm

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp -fv config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -fv dwm ${OBJ} dwm-${VERSION}.tar.gz

dist: clean
	mkdir -pv dwm-${VERSION}
	cp -Rv LICENSE Makefile README config.def.h config.mk\
		dwm.1 drw.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	tar -cfv dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rfv dwm-${VERSION}

install: all
	mkdir -pv ${DESTDIR}${PREFIX}/bin
	cp -fv dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -pv ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1
	mkdir -pv ${DESTDIR}${PREFIX}/bin
	cp -fv dwmautostart.sh ${DESTDIR}${PREFIX}/bin/dwmautostart.sh
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwmautostart.sh
	mkdir -pv ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/
	cp -fv dwm.png ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/dwm.png
	mkdir -pv ${DESTDIR}/usr/share/xsessions/
	cp -fv dwm.desktop ${DESTDIR}/usr/share/xsessions/dwm.desktop

uninstall:
	rm -fv ${DESTDIR}${PREFIX}/bin/dwm \
	${DESTDIR}${PREFIX}/bin/dwmautostart.sh \
  ${DESTDIR}/usr/share/icons/hicolor/48x48/apps/dwm.png \
  ${DESTDIR}/usr/share/xsessions/dwm.desktop \
	${DESTDIR}${MANPREFIX}/man1/dwm.1

.PHONY: all clean dist install uninstall
