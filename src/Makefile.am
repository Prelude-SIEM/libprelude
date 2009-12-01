SUBDIRS = libprelude-error include

AM_CPPFLAGS = @PCFLAGS@ -I$(srcdir)/include -I$(top_builddir) -I$(top_builddir)/src/include -I$(srcdir)/libprelude-error \
-I$(top_builddir)/libmissing -I$(top_srcdir)/libmissing $(LTDLINCL) @LIBPRELUDE_CFLAGS@ @LIBGNUTLS_CFLAGS@ @LIBGCRYPT_CFLAGS@ 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libprelude.pc
DISTCLEANFILES = $(pkgconfig_DATA)

libpreludeincludedir = $(includedir)/
libprelude_la_LIBADD = @LIBPRELUDE_LIBS@ $(LIBLTDL) $(top_builddir)/libmissing/libmissing.la libprelude-error/libprelude-error.la
libprelude_la_LDFLAGS = $(GETADDRINFO_LIB) $(GETHOSTNAME_LIB) $(HOSTENT_LIB) $(INET_NTOP_LIB) $(INET_PTON_LIB) $(LIBSOCKET) $(LTLIBINTL) $(LTLIBTHREAD) $(SERVENT_LIB) -no-undefined -version-info @LIBPRELUDE_SONAME@ -export-symbols-regex "^(idmef|_?prelude)_"
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
	idmef-message-print.c		\
	idmef-message-read.c		\
	idmef-message-write.c		\
	idmef-path.c			\
	idmef-time.c			\
	idmef-tree-wrap.c		\
	idmef-value.c			\
	idmef-value-type.c		



IDMEF_CRITERIA_STRING_LEX_O=idmef-criteria-string.lex.o
$(IDMEF_CRITERIA_STRING_LEX_O): idmef-criteria-string.lex.c
	$(MAKE) idmef-criteria-string.lex.o CFLAGS="$(CFLAGS) -include $(top_builddir)/config.h" IDMEF_CRITERIA_STRING_LEX_O=dummy-idmef-criteria-string.lex.o

IDMEF_CRITERIA_STRING_LEX_LO=idmef-criteria-string.lex.lo
$(IDMEF_CRITERIA_STRING_LEX_LO): idmef-criteria-string.lex.c
	$(MAKE) idmef-criteria-string.lex.lo CFLAGS="$(CFLAGS) -include $(top_builddir)/config.h" IDMEF_CRITERIA_STRING_LEX_LO=dummy-idmef-criteria-string.lex.lo

IDMEF_CRITERIA_STRING_YAC_O=idmef-criteria-string.yac.o
$(IDMEF_CRITERIA_STRING_YAC_O): idmef-criteria-string.yac.c
	$(MAKE) idmef-criteria-string.yac.o CFLAGS="$(CFLAGS) -include $(top_builddir)/config.h" IDMEF_CRITERIA_STRING_YAC_O=dummy-idmef-criteria-string.yac.o

IDMEF_CRITERIA_STRING_YAC_LO=idmef-criteria-string.yac.lo
$(IDMEF_CRITERIA_STRING_YAC_LO): idmef-criteria-string.yac.c
	$(MAKE) idmef-criteria-string.yac.lo CFLAGS="$(CFLAGS) -include $(top_builddir)/config.h" IDMEF_CRITERIA_STRING_YAC_LO=dummy-idmef-criteria-string.yac.lo

-include $(top_srcdir)/git.mk
