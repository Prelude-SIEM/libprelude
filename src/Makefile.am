SUBDIRS = include
DEFS = -DSENSORS_CONFIG_DIR=\"@sensors_config_dir@\"

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libmissing -I$(srcdir)/include 

libpreludeincludedir = $(includedir)/
lib_LTLIBRARIES = libprelude.la

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	extract.c			\
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
	prelude-ident.c			\
	sensor.c			\
	client-ident.c			\
	common.c


libprelude_la_LIBADD = ../libmissing/libmissing.a @LIBS@

