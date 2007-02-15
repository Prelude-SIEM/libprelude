AM_CPPFLAGS = 	-I$(top_builddir)/src/include -I$(top_srcdir)/src/include -I$(top_builddir)/libmissing -I$(top_srcdir)/libmissing 	\
		-I$(top_builddir)/src/libprelude-error @libprelude_cflags@ @LIBGNUTLS_CFLAGS@		\
		@PRELUDE_ADDUSER_CFLAGS@ -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\"

bin_PROGRAMS = prelude-adduser
prelude_adduser_LDADD = $(top_builddir)/src/config-engine.lo \
			$(top_builddir)/src/variable.lo \
		        $(top_builddir)/src/libprelude.la \
			@PRELUDE_ADDUSER_LIBS@ \
			../libmissing/libmissing.la


prelude_adduser_SOURCES = 		\
	prelude-adduser.c		\
	server.c			\
	tls-register.c			

noinst_HEADERS = tls-register.h server.h
