ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src tests

test:
	make check || { cat tests/test-suite.log && false; }
