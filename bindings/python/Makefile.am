###
# We don't use libtool to compile Prelude.so because libtool --mode=install does not
# allow us to install Prelude.so in a directory not in ${prefix}/*
###

DEFS=-I$(top_srcdir) -I$(top_srcdir)/src/include -D_GNU_SOURCE @PYTHON_CFLAGS@
CFLAGS=
LINK = $(CC) -o $@

if HAVE_PYTHON

bin_PROGRAMS = _prelude.so

_prelude_so_LDFLAGS = -shared -L$(top_srcdir)/src/.libs
_prelude_so_SOURCES = _prelude.c
_prelude_so_LDADD = @libprelude_libs@ -lprelude
noinst_PROGRAMS = _prelude.so

endif

install-exec-local:
if HAVE_PYTHON
	$(INSTALL) -d -m 755 $(DESTDIR)/$(python_dir)
	$(INSTALL) -m 644 prelude.py $(DESTDIR)/$(python_dir)
	$(INSTALL) -m 644 _prelude.so $(DESTDIR)/$(python_dir)
endif
