DEFS = -DSENSORS_CONFIG_DIR=\"@sensors_config_dir@\"
INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)

bin_PROGRAMS = sensor-adduser
sensor_adduser_LDADD = @SSL_LIBS@ ../src/libprelude.la
sensor_adduser_SOURCES = sensor-adduser.c ssl-register.c

noinst_HEADERS=ssl-register.h





