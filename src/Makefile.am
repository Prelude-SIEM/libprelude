SUBDIRS = include
DEFS = -DCONFIG_DIR=\"@configdir@\"

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libmissing -I$(srcdir)/include 

libpreludeincludedir = $(includedir)/
lib_LTLIBRARIES = libprelude.la

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	plugin-common.c			\
	daemonize.c			\
	ssl.c				\
	ssl-gencrypto.c			\
	ssl-registration-msg.c		\
	string-matching.c		\
	timer.c				\
	variable.c			\
	prelude-auth.c			\
	prelude-client.c		\
	prelude-client-mgr.c		\
	prelude-getopt.c		\
	prelude-io.c			\
	prelude-message.c		\
	prelude-async.c			\
	sensor.c			\
	common.c


libprelude_la_LIBADD = ../libmissing/libmissing.a @LIBS@

