if HAVE_PYTHON

EXTRA_DIST = libprelude-python.i _prelude.c prelude.py
PYLIBVER ?= python$(PYTHON_VERSION)
python_PYTHON = prelude.py

pyexec_LTLIBRARIES = _prelude.la
_prelude_la_CPPFLAGS = $(PYTHON_INCLUDES) -I@top_builddir@ -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -I@top_builddir@/src/libprelude-error 
_prelude_la_LDFLAGS = -module -avoid-version
_prelude_la_LIBADD = $(top_builddir)/src/.libs/libprelude.la
_prelude_la_LIBS = $(top_builddir)/src/.libs/libprelude.la
_prelude_la_HEADERS = $(top_builddir)/config.h
nodist__prelude_la_SOURCES = _prelude.c

_prelude.c: $(top_srcdir)/bindings/low-level/libprelude.i libprelude-python.i
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/bindings -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -o $@ -python -noproxy -interface _prelude -module _prelude $(top_srcdir)/bindings/low-level/libprelude.i
endif

endif
