ACLOCAL_AMFLAGS = -I m4 -I libmissing/m4
SUBDIRS = libltdl libmissing src prelude-adduser bindings docs

EXTRA_DIST = client.conf global.conf tls.conf

bin_SCRIPTS = $(top_builddir)/libprelude-config


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-local:
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_spool_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/default;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/tls;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/tls/keys;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/tls/client;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/tls/server;
	$(INSTALL) -d -m 755 $(DESTDIR)$(prelude_config_dir)/analyzerid;
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

	@if test -f $(DESTDIR)$(prelude_config_dir)/tls/tls.conf; then								\
		echo;														\
		echo "********************************************************************************";          		\
                echo;														\
                echo "$(DESTDIR)$(prelude_config_dir)/tls/tls.conf already exist...";						\
                echo "Installing default configuration in $(DESTDIR)$(prelude_config_dir)/tls/tls.conf-dist";			\
                echo;														\
                echo "********************************************************************************";			\
		echo;														\
		$(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(prelude_config_dir)/tls/tls.conf-dist;			\
        else															\
                $(INSTALL) -m 644 $(top_srcdir)/tls.conf $(DESTDIR)$(prelude_config_dir)/tls;					\
	fi


uninstall-local:
	rm -f $(DESTDIR)$(prelude_config_dir)/tls/tls.conf
	rm -f $(DESTDIR)$(prelude_config_dir)/default/client.conf
	rm -f $(DESTDIR)$(prelude_config_dir)/default/global.conf
