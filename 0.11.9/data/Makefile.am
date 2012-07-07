
SUBDIRS = pixmaps player maps gfx music tileset

install: install-recursive
	@if test ! -d player; then \
		echo ""; \
		echo ""; \
		echo "****************************************"; \
		echo "*  Please download the Data Package,   *"; \
		echo "* so you can complete the installation *"; \
		echo "*   URL: http://www.bomberclone.de     *"; \
		echo "****************************************"; \
		echo ""; \
		echo ""; \
		exit 1; \
	fi;
	@if test ! -d ${DESTDIR}; then \
		mkdir -p ${DESTDIR}; \
	fi;
	@if test ! -d ${DESTDIR}@PACKAGE_DATA_DIR@; then \
		mkdir -p ${DESTDIR}@PACKAGE_DATA_DIR@; \
	fi;
#	cp -rf player ${DESTDIR}@PACKAGE_DATA_DIR@
#	cp -rf maps ${DESTDIR}@PACKAGE_DATA_DIR@
#	cp -rf gfx ${DESTDIR}@PACKAGE_DATA_DIR@
#	cp -rf tileset ${DESTDIR}@PACKAGE_DATA_DIR@
#	cp -rf music ${DESTDIR}@PACKAGE_DATA_DIR@

uninstall: uninstall-recursive
	rm -rf ${DESTDIR}@PACKAGE_DATA_DIR@

EXTRA_DIST =
