DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
ACLOCAL_AMFLAGS = -I m4 -I libmissing/m4

SUBDIRS = libltdl libmissing m4 src prelude-admin bindings docs tests
DIST_SUBDIRS=$(SUBDIRS)

EXTRA_DIST = HACKING.README LICENSE.README tls.conf

bin_SCRIPTS = $(top_builddir)/libprelude-config

distuninstallcheck_listfiles = find . -type f -print | $(GREP) -v 'perl' | $(GREP) -v '3pm' | $(GREP) -v 'egg-info'

includedir = $(prefix)/include/libprelude
include_HEADERS = prelude-config.h

MOSTLYCLEANFILES = prelude-config.h
BUILT_SOURCES = prelude-config.h

MAINTAINERCLEANFILES = \
        $(srcdir)/INSTALL \
        $(srcdir)/aclocal.m4 \
        $(srcdir)/autoscan.log \
        $(srcdir)/compile \
        $(srcdir)/config.guess \
        $(srcdir)/config.h.in \
        $(srcdir)/config.sub \
        $(srcdir)/configure.scan \
        $(srcdir)/depcomp \
        $(srcdir)/install-sh \
        $(srcdir)/ltmain.sh \
        $(srcdir)/missing \
        $(srcdir)/mkinstalldirs \
        $(srcdir)/omf.make \
        $(srcdir)/xmldocs.make \
	$(srcdir)/ylwrap \
        $(srcdir)/gtk-doc.make \
        $(srcdir)/ChangeLog \
        `find "$(srcdir)" -type f -name Makefile.in -print`

prelude-config.h: $(top_builddir)/config.h
	rm -f prelude-config.h
	echo "/* Used from libprelude headers */" >> prelude-config.h
	$(GREP) "HAVE_VARIADIC_MACROS"          < $(top_builddir)/config.h >> prelude-config.h
	$(GREP) "PRELUDE_ALIGNED_ACCESS"        < $(top_builddir)/config.h >> prelude-config.h
	$(GREP) "PRELUDE_WORDS_BIGENDIAN"       < $(top_builddir)/config.h >> prelude-config.h
	$(GREP) "__PRELUDE_FUNC__"              < $(top_builddir)/config.h >> prelude-config.h
	@echo "#ifndef TIME_WITH_SYS_TIME"      >> prelude-config.h
	$(GREP) "TIME_WITH_SYS_TIME"            < $(top_builddir)/config.h >> prelude-config.h
	@echo "#endif"                          >> prelude-config.h
	@echo "#ifndef HAVE_UID_T"              >> prelude-config.h
	$(GREP) "HAVE_UID_T"                    < $(top_builddir)/config.h >> prelude-config.h
	@echo "#endif"                          >> prelude-config.h
	@echo "#ifndef HAVE_GID_T"              >> prelude-config.h
	$(GREP) "HAVE_GID_T"                    < $(top_builddir)/config.h >> prelude-config.h
	@echo "#endif"                          >> prelude-config.h



#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-local:
	$(INSTALL) -d -m 711 $(DESTDIR)$(PRELUDE_SPOOL_DIR);
	$(INSTALL) -d -m 755 $(DESTDIR)$(PRELUDE_CONFIG_DIR);
	$(INSTALL) -d -m 755 $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default;
	$(INSTALL) -d -m 711 $(DESTDIR)$(PRELUDE_CONFIG_DIR)/profile;
	@if test -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/client.conf; then							\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/client.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/client.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_builddir)/client.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/client.conf-dist;		\
        else															\
                $(INSTALL) -m 644 $(top_builddir)/client.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default;				\
        fi

	@if test -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/global.conf; then							\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/global.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/global.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_builddir)/global.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/global.conf-dist;		\
        else															\
                $(INSTALL) -m 644 $(top_builddir)/global.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default;				\
        fi															

	@if test -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/idmef-client.conf; then						\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/idmef-client.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/idmef-client.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_builddir)/idmef-client.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/idmef-client.conf-dist;\
        else															\
                $(INSTALL) -m 644 $(top_builddir)/idmef-client.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default;			\
        fi

	@if test -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/tls.conf; then								\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/tls.conf already exist...";						\
                echo "Installing default configuration in $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/tls.conf-dist";			\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/tls.conf-dist;			\
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default;					\
	fi


uninstall-local:
	rm -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/client.conf
	rm -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/global.conf
	rm -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/idmef-client.conf
	rm -f $(DESTDIR)$(PRELUDE_CONFIG_DIR)/default/tls.conf


dist-hook:
	@if test -d "$(srcdir)/.git"; then	\
		echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && \
		  echo '# Generated by Makefile. Do not edit.'; echo; \
		  $(top_srcdir)/missing --run git log --stat ) > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
		|| ( rm -f ChangeLog.tmp ; \
		echo Failed to generate ChangeLog >&2 ); \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi

-include $(top_srcdir)/git.mk
