SUBDIRS = include
DEFS = -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\" -DPRELUDE_SPOOL_DIR=\"@prelude_spool_dir@\" -DPRELUDE_IDENT_FILE=\"@prelude_ident_file@\" @libprelude_cflags@ @pthread_cflags@
INCLUDES = -I$(top_srcdir) -I$(srcdir)/include @INCLTDL@

libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @LIBLTDL@
libprelude_la_LDFLAGS = -version-info @libprelude_soname@
lib_LTLIBRARIES = libprelude.la

if HAVE_SSL
SSL_SRCS=ssl.c ssl-gencrypto.c ssl-registration-msg.c ssl-settings.c
endif

AM_YFLAGS = -d
LEX_OUTPUT_ROOT = lex.yy

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	daemonize.c			\
	timer.c				\
	variable.c			\
	prelude-auth.c			\
	prelude-client.c		\
	prelude-connection.c		\
	prelude-connection-mgr.c	\
	prelude-getopt.c		\
	prelude-getopt-wide.c		\
	prelude-io.c			\
	prelude-message.c		\
	prelude-message-buffered.c	\
	prelude-async.c			\
	prelude-failover.c		\
	prelude-ident.c			\
	prelude-plugin.c		\
	prelude-hash.c			\
	prelude-log.c			\
	prelude-inet.c			\
	prelude-strbuf.c		\
	client-ident.c			\
	ntp.c				\
	idmef-util.c			\
	idmef-time.c			\
	idmef-data.c			\
	idmef-string.c			\
	idmef-tree-print.c		\
	idmef-tree-to-string.c		\
	idmef-tree-wrap.c		\
	idmef-type.c			\
	idmef-object.c			\
	idmef-value.c			\
	idmef-value-object.c		\
	idmef-value-type.c		\
	idmef-object-value.c		\
	idmef-criteria.c		\
	idmef-criteria-string.yac.y	\
	idmef-criteria-string.lex.l	\
	idmef-criterion-value.c		\
	idmef-message-write.c		\
	idmef-message-read.c		\
	idmef-message.c			\
	common.c			\
	$(SSL_SRCS)


