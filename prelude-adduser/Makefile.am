INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)
DEFS = -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\"

bin_PROGRAMS = prelude-adduser
prelude_adduser_LDFLAGS = -export-dynamic
prelude_adduser_LDADD = $(top_builddir)/src/libprelude.la @libprelude_libs@ -lgnutls-extra

prelude_adduser_SOURCES = 		\
	prelude-adduser.c		\
	server.c			\
	tls-register.c			


noinst_HEADERS = tls-register.h server.h

