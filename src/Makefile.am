SUBDIRS = include
DEFS = -DSENSORS_CONFIG_DIR=\"@sensors_config_dir@\" -DSENSORS_SPOOL_DIR=\"@sensors_spool_dir@\" -DSENSORS_IDENT_FILE=\"@sensors_ident_file@\" @libprelude_cflags@
INCLUDES = -I$(top_srcdir) -I$(srcdir)/include @INCLTDL@

libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @LIBLTDL@
lib_LTLIBRARIES = libprelude.la

if HAVE_SSL
 SSL_SRCS=ssl.c ssl-gencrypto.c ssl-registration-msg.c ssl-settings.c
endif


libprelude_la_SOURCES = 		\
	config-engine.c 		\
	plugin-common.c			\
	daemonize.c			\
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
	common.c			\
	$(SSL_SRCS)


