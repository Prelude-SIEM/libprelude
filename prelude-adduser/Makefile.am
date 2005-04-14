AM_CPPFLAGS = 	-I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_srcdir)/libmissing 	\
		-I$(top_builddir)/src/libprelude-error @libprelude_cflags@ @LIBGNUTLS_CFLAGS@		\
		@LIBGNUTLS_EXTRA_CFLAGS@ -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\"

bin_PROGRAMS = prelude-adduser
prelude_adduser_LDADD = $(top_builddir)/src/libprelude.la @prelude_adduser_libs@

prelude_adduser_SOURCES = 		\
	prelude-adduser.c		\
	server.c			\
	tls-register.c			


noinst_HEADERS = tls-register.h server.h
