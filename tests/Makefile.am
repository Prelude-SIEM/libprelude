TESTS = idmef idmef-criteria idmef-path idmef-value prelude-client prelude-string
check_PROGRAMS = $(TESTS)
LDADD = $(top_builddir)/src/libprelude.la
AM_CPPFLAGS = -I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_builddir)/src/libprelude-error

idmef_value_LDADD = $(top_builddir)/src/idmef-value.lo $(LDADD)

check-local:
	cd $(top_srcdir)/libmissing/tests && $(MAKE) check
