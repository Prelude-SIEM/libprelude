AM_CPPFLAGS = @PCFLAGS@ -I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_builddir)/libmissing -I$(top_srcdir)/libmissing -I$(top_builddir)/src/libprelude-error @LIBPRELUDE_CFLAGS@ @PRELUDE_ADMIN_CFLAGS@

bin_PROGRAMS = prelude-admin
bin_SCRIPTS = prelude-adduser

prelude_admin_LDADD = $(top_builddir)/src/config-engine.lo \
			$(top_builddir)/src/variable.lo \
		        $(top_builddir)/src/libprelude.la \
			@PRELUDE_ADMIN_LIBS@ \
			../libmissing/libmissing.la $(GETADDRINFO_LIB) $(GETHOSTNAME_LIB) $(HOSTENT_LIB) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBSOCKET) $(LIB_POLL) $(LIB_SELECT) $(LTLIBINTL) $(LTLIBTHREAD) $(SERVENT_LIB)


prelude_admin_SOURCES = 		\
	prelude-admin.c		\
	server.c			\
	tls-register.c			

noinst_HEADERS = tls-register.h server.h

-include $(top_srcdir)/git.mk
