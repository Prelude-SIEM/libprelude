if HAVE_EASY_BINDINGS

EXTRA_DIST = libpreludecpp-lua.i PreludeEasy.cxx

if HAVE_LUA

AM_CPPFLAGS = -I@top_srcdir@ -I@top_builddir@/src/include -I@top_srcdir@/src/include -I@top_builddir@/src/libprelude-error -I@top_srcdir@/bindings/c++/include @LUA_CFLAGS@ -I@top_srcdir@/libmissing -I@top_builddir@/libmissing

PreludeEasy_la_LDFLAGS = -module -avoid-version @LUA_LIBS@
PreludeEasy_la_LIBADD = $(top_builddir)/bindings/c++/.libs/libpreludecpp.la
PreludeEasy_la_SOURCES = PreludeEasy.cxx
PreludeEasydir = $(libdir)
PreludeEasy_LTLIBRARIES = PreludeEasy.la

PreludeEasy.cxx: $(top_srcdir)/bindings/c++/include/*.hxx $(top_srcdir)/bindings/libpreludecpp.i libpreludecpp-lua.i 
if HAVE_SWIG
	$(SWIG) -c++ -I$(top_srcdir)/bindings -I$(top_srcdir)/bindings/c++/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -lua -o $@ libpreludecpp.i
endif


endif

endif

-include $(top_srcdir)/git.mk
