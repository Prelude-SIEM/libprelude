SUBDIRS = libmissing src sensor-adduser docs

includedir=$(prefix)/libprelude/include
include_HEADERS=config.h

bin_SCRIPTS = libprelude-config


#
# Install default sensors configuration with mode 644
# every sensors should be able to read it.
#
# Spool directory for report saving is mode 

install-data-hook:
	install -d -m 755 $(sensors_config_dir); \
	install -d -m 1733 /var/spool/prelude; \
	install -m 644 sensors-default.conf $(sensors_config_dir)/sensors-default.conf;


