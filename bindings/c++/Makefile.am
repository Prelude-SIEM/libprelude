if HAVE_EASY_BINDINGS

SUBDIRS = include

lib_LTLIBRARIES = libpreludecpp.la
libpreludecpp_ladir = ${prefix}/lib
libpreludecpp_la_SOURCES = \
	prelude-error.cxx \
	idmef.cxx \
	idmef-criteria.cxx \
	idmef-path.cxx \
	idmef-time.cxx \
	idmef-value.cxx \
	prelude.cxx	\
	prelude-client.cxx \
	prelude-client-easy.cxx \
	prelude-client-profile.cxx \
	prelude-connection.cxx	\
	prelude-connection-pool.cxx \
	prelude-log.cxx

libpreludecpp_la_LDFLAGS = -no-undefined -version-info @LIBPRELUDECPP_SONAME@
libpreludecpp_la_LIBADD  = $(top_builddir)/src/.libs/libprelude.la
libpreludecpp_la_CPPFLAGS = -I$(top_builddir) -I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_builddir)/src/libprelude-error -I$(top_builddir)/bindings/c++/include -I$(top_srcdir)/bindings/c++/include 

endif

-include $(top_srcdir)/git.mk
