SUBDIRS = include

AM_CPPFLAGS = -I$(top_builddir)/src/include @PTHREAD_CFLAGS@ @INCLTDL@ @libprelude_cflags@ -I$(srcdir)/include \
	      -DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\" -DPRELUDE_SPOOL_DIR=\"@prelude_spool_dir@\"


libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @libprelude_libs@ @LIBLTDL@
libprelude_la_LDFLAGS = @PTHREAD_CFLAGS@ -no-undefined -version-info @libprelude_soname@
lib_LTLIBRARIES = libprelude.la

AM_YFLAGS = -d
LEX_OUTPUT_ROOT = lex.yy

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	daemonize.c			\
	timer.c				\
	variable.c			\
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
	tls-auth.c			\
	tls-util.c			


