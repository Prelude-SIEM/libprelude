AM_CPPFLAGS = 	@libprelude_cflags@ -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\" \
		@LIBGNUTLS_EXTRA_CFLAGS@ -I$(top_srcdir)/src/include

bin_PROGRAMS = prelude-adduser
prelude_adduser_LDADD = @libprelude_libs@ @LIBGNUTLS_EXTRA_LIBS@ $(top_builddir)/src/libprelude.la

prelude_adduser_SOURCES = 		\
	prelude-adduser.c		\
	server.c			\
	tls-register.c			


noinst_HEADERS = tls-register.h server.h
