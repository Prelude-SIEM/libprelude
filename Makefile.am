SUBDIRS = libmissing src docs

includedir=$(prefix)/libprelude/include
include_HEADERS=config.h

bin_SCRIPTS = libprelude-config


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-hook:
	install -d -m 755 $(sensors_config_dir);         	   \
	install -d -m 1733 /var/lib/prelude-sensors/plaintext;     \
	install -d -m 1733 /var/lib/prelude-sensors/ssl;	   \
	install -d -m 1733 /var/lib/prelude-sensors/idents;	   \
	install -d -m 1733 /var/spool/prelude-sensors;		   \
	install -m 644 sensors-default.conf $(sensors_config_dir)/sensors-default.conf;


