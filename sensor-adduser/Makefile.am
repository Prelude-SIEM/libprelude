DEFS = -DCONFIG_DIR=\"@configdir@\"
INCLUDES = -I$(top_srcdir)/src/include

bin_PROGRAMS = sensor-adduser
sensor_adduser_LDADD = -lssl $(top_srcdir)/src/libprelude.la
sensor_adduser_SOURCES = sensor-adduser.c ssl-register.c


