SUBDIRS = libprelude-error include

AM_CPPFLAGS = @REGEX_CFLAGS@ -I$(srcdir)/include -I$(top_builddir)/src/include 	\
		-I$(srcdir)/libprelude-error -I$(top_srcdir)/libmissing @INCLTDL@ @libprelude_cflags@	\
		-DPRELUDE_CONFIG_DIR=\"@prelude_config_dir@\" -DPRELUDE_SPOOL_DIR=\"@prelude_spool_dir@\"


libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @libprelude_libs@ @LIBLTDL@
libprelude_la_LDFLAGS = -no-undefined -version-info @libprelude_soname@ \
			../libmissing/libmissing.la libprelude-error/libprelude-error.la
lib_LTLIBRARIES = libprelude.la

AM_YFLAGS = -d
LEX_OUTPUT_ROOT = lex.yy

libprelude_la_SOURCES = 		\
	common.c			\
	config-engine.c 		\
	daemonize.c			\
	ntp.c				\
	tls-auth.c			\
	tls-util.c			\
	variable.c			\
	prelude.c			\
	prelude-async.c			\
	prelude-client.c		\
	prelude-client-profile.c	\
	prelude-connection.c		\
	prelude-connection-pool.c	\
	prelude-failover.c		\
	prelude-hash.c			\
	prelude-ident.c			\
	prelude-io.c			\
	prelude-log.c			\
	prelude-msg.c			\
	prelude-msgbuf.c		\
	prelude-option.c		\
	prelude-option-wide.c		\
	prelude-plugin.c		\
	prelude-string.c		\
	prelude-timer.c			\
	idmef-additional-data.c		\
	idmef-class.c			\
	idmef-criteria.c		\
	idmef-criteria-string.yac.y	\
	idmef-criteria-string.lex.l	\
	idmef-criterion-value.c		\
	idmef-data.c			\
	idmef-message-print.c		\
	idmef-message-read.c		\
	idmef-message-write.c		\
	idmef-path.c			\
	idmef-time.c			\
	idmef-tree-wrap.c		\
	idmef-value.c			\
	idmef-value-type.c		

