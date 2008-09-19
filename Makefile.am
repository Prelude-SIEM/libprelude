DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --without-system-ltdl --enable-easy-bindings
ACLOCAL_AMFLAGS = -I m4 -I libmissing/m4

if HAVE_SYSTEM_LIBLTDL
SUBDIRS = libmissing m4 src prelude-admin bindings docs tests
else
SUBDIRS = libltdl libmissing m4 src prelude-admin bindings docs tests
endif

DIST_SUBDIRS=$(SUBDIRS)

EXTRA_DIST = HACKING.README LICENSE.README tls.conf

bin_SCRIPTS = $(top_builddir)/libprelude-config

distuninstallcheck_listfiles = find . -type f -print | $(GREP) -v 'perl' | $(GREP) -v '3pm' | $(GREP) -v 'egg-info'

includedir = $(prefix)/include/libprelude
include_HEADERS = prelude-config.h

MOSTLYCLEANFILES = prelude-config.h
BUILT_SOURCES = prelude-config.h

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
