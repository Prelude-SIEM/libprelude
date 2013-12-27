if HAVE_EASY_BINDINGS
if HAVE_RUBY

EXTRA_DIST = libpreludecpp-ruby.i PreludeEasy.cxx

rbexec_LTLIBRARIES = PreludeEasy.la
PreludeEasy_la_CPPFLAGS = $(RUBY_INCLUDES) $(RUBY_CCFLAGS) -I@top_builddir@ -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -I@top_builddir@/src/libprelude-error -I$(top_srcdir)/bindings/c++/include -I@top_srcdir@/libmissing -I@top_builddir@/libmissing
PreludeEasy_la_LDFLAGS = -module -avoid-version
PreludeEasy_la_LIBADD = $(top_builddir)/bindings/c++/.libs/libpreludecpp.la $(RUBY_LIBS)
nodist_PreludeEasy_la_SOURCES = PreludeEasy.cxx

PreludeEasy.cxx: $(top_srcdir)/bindings/c++/include/*.hxx $(top_srcdir)/bindings/libpreludecpp.i libpreludecpp-ruby.i 
if HAVE_SWIG
	$(SWIG) -c++ -I$(top_srcdir)/bindings -I$(top_srcdir)/bindings/c++/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -ruby -o $@ $(top_srcdir)/bindings/libpreludecpp.i
endif

endif
endif

-include $(top_srcdir)/git.mk
