AM_CPPFLAGS = 	-I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_builddir)/libmissing -I$(top_srcdir)/libmissing 	\
		-I$(top_builddir)/src/libprelude-error @libprelude_cflags@ @LIBGNUTLS_CFLAGS@		\
		@PRELUDE_ADMIN_CFLAGS@ -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\"

bin_PROGRAMS = prelude-admin

dist_bin_SCRIPTS = prelude-adduser

prelude_admin_LDADD = $(top_builddir)/src/config-engine.lo \
			$(top_builddir)/src/variable.lo \
		        $(top_builddir)/src/libprelude.la \
			@PRELUDE_ADMIN_LIBS@ \
			../libmissing/libmissing.la


prelude_admin_SOURCES = 		\
	prelude-admin.c		\
	server.c			\
	tls-register.c			

noinst_HEADERS = tls-register.h server.h
