if HAVE_EASY_BINDINGS
if HAVE_PYTHON

EXTRA_DIST = libpreludecpp-python.i PreludeEasy.py _PreludeEasy.cxx
PYLIBVER ?= python$(PYTHON_VERSION)
python_PYTHON = PreludeEasy.py

pyexec_LTLIBRARIES = _PreludeEasy.la
_PreludeEasy_la_CPPFLAGS = $(PYTHON_INCLUDES) -I@top_builddir@ -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -I@top_builddir@/src/libprelude-error -I$(top_srcdir)/bindings/c++/include
_PreludeEasy_la_LDFLAGS = -module -avoid-version
_PreludeEasy_la_LIBADD = $(top_builddir)/bindings/c++/.libs/libpreludecpp.la
_PreludeEasy_la_HEADERS = $(top_builddir)/config.h
nodist__PreludeEasy_la_SOURCES = _PreludeEasy.cxx

_PreludeEasy.cxx PreludeEasy.py: $(top_srcdir)/bindings/c++/include/*.hxx $(top_srcdir)/bindings/libpreludecpp.i libpreludecpp-python.i 
if HAVE_SWIG
	$(SWIG) -c++ -I$(top_srcdir)/bindings -I$(top_srcdir)/bindings/c++/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -python -o $@ ../libpreludecpp.i
endif

endif
endif
