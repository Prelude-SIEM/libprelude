SUBDIRS = include
DEFS = -DSENSORS_CONFIG_DIR=\"@sensors_config_dir@\" @libprelude_cflags@
INCLUDES = -I$(top_srcdir) -I$(srcdir)/include 

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
	ssl-register.c			\
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
	prelude-path.c			\
	sensor.c			\
	client-ident.c			\
	common.c			\
	idmef-tree-func.c		\
	idmef-msg-send.c		

