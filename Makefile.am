DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

ACLOCAL_AMFLAGS = -I m4 -I libmissing/m4
SUBDIRS = libltdl libmissing m4 src prelude-adduser bindings docs

EXTRA_DIST = HACKING.README LICENSE.README client.conf global.conf idmef-client.conf tls.conf

bin_SCRIPTS = $(top_builddir)/libprelude-config

distuninstallcheck_listfiles = find . -type f -print | grep -v 'perl' | grep -v '3pm'

includedir = $(prefix)/include/libprelude
include_HEADERS = prelude-config.h

MOSTLYCLEANFILES = prelude-config.h
BUILT_SOURCES = prelude-config.h

prelude-config.h: $(top_builddir)/config.h
	rm -f prelude-config.h
	$(SED) -e 's|\(#define PACKAGE.*\)|/* do not publish: \1 */|' \
	-e 's|\(#define VERSION.*\)|/* do not publish: \1 */|' < $(top_builddir)/config.h > prelude-config.h


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-local:
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_spool_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/default;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/profile;
	@if test -f $(DESTDIR)$(prelude_config_dir)/default/client.conf; then							\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(prelude_config_dir)/default/client.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(prelude_config_dir)/default/client.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/client.conf $(DESTDIR)$(prelude_config_dir)/default/client.conf-dist;		\
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/client.conf $(DESTDIR)$(prelude_config_dir)/default;				\
        fi

	@if test -f $(DESTDIR)$(prelude_config_dir)/default/global.conf; then							\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(prelude_config_dir)/default/global.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(prelude_config_dir)/default/global.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/global.conf $(DESTDIR)$(prelude_config_dir)/default/global.conf-dist;		\
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/global.conf $(DESTDIR)$(prelude_config_dir)/default;				\
        fi															

	@if test -f $(DESTDIR)$(prelude_config_dir)/default/idmef-client.conf; then						\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(prelude_config_dir)/default/idmef-client.conf already exist...";				\
                echo "Installing default configuration in $(DESTDIR)$(prelude_config_dir)/default/idmef-client.conf-dist";	\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/idmef-client.conf $(DESTDIR)$(prelude_config_dir)/default/idmef-client.conf-dist; \
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/idmef-client.conf $(DESTDIR)$(prelude_config_dir)/default;			\
        fi

	@if test -f $(DESTDIR)$(prelude_config_dir)/default/tls.conf; then							\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(prelude_config_dir)/default/tls.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(prelude_config_dir)/default/tls.conf-dist";		\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(prelude_config_dir)/default/tls.conf-dist;			\
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(prelude_config_dir)/default;				\
	fi


uninstall-local:
	rm -f $(DESTDIR)$(prelude_config_dir)/default/client.conf
	rm -f $(DESTDIR)$(prelude_config_dir)/default/global.conf
	rm -f $(DESTDIR)$(prelude_config_dir)/default/idmef-client.conf
	rm -f $(DESTDIR)$(prelude_config_dir)/default/tls.conf
