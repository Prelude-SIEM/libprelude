INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)
DEFS = -DPRELUDE_IDENT_FILE=\"@prelude_ident_file@\"
bin_PROGRAMS = sensor-adduser

sensor_adduser_LDFLAGS = -export-dynamic
sensor_adduser_LDADD = $(top_builddir)/src/libprelude.la @libprelude_libs@

if HAVE_SSL
SSL_SRCS=ssl-register.c
endif

sensor_adduser_SOURCES = 		\
	sensor-adduser.c		\
	plaintext.c			\
	$(SSL_SRCS)


noinst_HEADERS = plaintext.h ssl-register.h
