SUBDIRS = libltdl src sensor-adduser bindings docs

includedir=$(prefix)/libprelude/include
include_HEADERS=config.h

EXTRA_DIST = COPYING.OpenSSL sensors-default.conf
bin_SCRIPTS = $(top_srcdir)/libprelude-config


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-local:
	$(INSTALL) -d -m 755 $(DESTDIR)$(sensors_spool_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(sensors_config_dir);
	$(INSTALL) -d -m 755 $(DESTDIR)$(sensors_config_dir)/ssl;
	$(INSTALL) -d -m 755 $(DESTDIR)$(sensors_config_dir)/plaintext;
	@if test -f $(DESTDIR)$(sensors_config_dir)/sensors-default.conf; then                                   		\
		echo "********************************************************************************";          		\
                echo;                                                                                             		\
                echo "$(DESTDIR)$(sensors_config_dir)/sensors-default.conf already exist...";					\
                echo "Installing default configuration in $(DESTDIR)$(sensors_config_dir)/sensors-default.conf-dist";		\
                echo;                                                                                             		\
                echo "********************************************************************************";          		\
		$(INSTALL) -m 644 $(top_srcdir)/sensors-default.conf $(DESTDIR)$(sensors_config_dir)/sensors-default.conf-dist;\
        else                                                                                                      		\
                $(INSTALL) -m 644 $(top_srcdir)/sensors-default.conf $(DESTDIR)$(sensors_config_dir)/;        			\
        fi


