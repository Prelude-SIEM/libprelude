SUBDIRS = libprelude-error jsmn include

AM_CPPFLAGS = @PCFLAGS@ -I$(srcdir)/include -I$(top_builddir) -I$(top_builddir)/src/include -I$(srcdir)/libprelude-error \
-I$(top_builddir)/libmissing -I$(top_srcdir)/libmissing $(LTDLINCL) @LIBPRELUDE_CFLAGS@ @LIBGNUTLS_CFLAGS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libprelude.pc
DISTCLEANFILES = $(pkgconfig_DATA) idmef-criteria-string.yac.h idmef-criteria-string.lex.c idmef-criteria-string.yac.c

libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @LIBPRELUDE_LIBS@ $(LIBLTDL) $(top_builddir)/libmissing/libmissing.la libprelude-error/libprelude-error.la
libprelude_la_LDFLAGS = $(GETADDRINFO_LIB) $(GETHOSTNAME_LIB) $(HOSTENT_LIB) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBSOCKET) $(LIB_POLL) $(LIB_SELECT) $(LTLIBINTL) $(LTLIBTHREAD) $(SERVENT_LIB) -no-undefined -version-info @LIBPRELUDE_SONAME@ -export-symbols-regex "^(idmef|_?prelude)_"
lib_LTLIBRARIES = libprelude.la
BUILT_SOURCES = $(LTDLDEPS) 

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
	prelude-thread.c		\
	idmef-additional-data.c		\
	idmef-class.c			\
	idmef-criteria.c		\
	idmef-criteria-string.yac.y	\
	idmef-criteria-string.lex.l	\
	idmef-criterion-value.c		\
	idmef-data.c			\
	idmef-message-helpers.c		\
	idmef-message-read-json.c	\
	idmef-message-print.c		\
        idmef-message-print-json.c	\
	idmef-message-read.c		\
	idmef-message-write.c		\
	idmef-object.c			\
	idmef-path.c			\
	idmef-time.c			\
	idmef-tree-wrap.c		\
	idmef-value.c			\
	idmef-value-type.c		

-include $(top_srcdir)/git.mk
