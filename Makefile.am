SUBDIRS = src sensor-adduser docs

includedir=$(prefix)/libprelude/include
include_HEADERS=config.h

EXTRA_DIST = sensors-default.conf

bin_SCRIPTS = $(top_srcdir)/libprelude-config


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-hook:
	install -d -m 755 $(sensors_config_dir);         	   \
	install -d -m 755 /var/lib/prelude-sensors/plaintext;      \
	install -d -m 755 /var/lib/prelude-sensors/ssl;	  	   \
	install -d -m 755 /var/spool/prelude-sensors;		   \
	install -d -m 1733 /var/lib/prelude-sensors/idents;	   \
	install -m 644 $(top_srcdir)/sensors-default.conf $(sensors_config_dir)/sensors-default.conf;


