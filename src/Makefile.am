SUBDIRS = include
DEFS = -DSENSORS_CONFIG_DIR=\"@sensors_config_dir@\" @libprelude_cflags@
INCLUDES = -I$(top_srcdir) -I$(srcdir)/include 

libpreludeincludedir = $(includedir)/
lib_LTLIBRARIES = libprelude.la

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	plugin-common.c			\
	daemonize.c			\
	ssl.c				\
	ssl-gencrypto.c			\
	ssl-registration-msg.c		\
	ssl-settings.c			\
	string-matching.c		\
	timer.c				\
	variable.c			\
	prelude-auth.c			\
	prelude-client.c		\
	prelude-client-mgr.c		\
	prelude-getopt.c		\
	prelude-io.c			\
	prelude-message.c		\
	prelude-message-buffered.c	\
	prelude-async.c			\
	prelude-ident.c			\
	prelude-path.c			\
	prelude-log.c			\
	sensor.c			\
	client-ident.c			\
	idmef-tree-func.c		\
	idmef-msg-send.c		\
	common.c

