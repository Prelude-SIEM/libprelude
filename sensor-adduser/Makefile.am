INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)

bin_PROGRAMS = sensor-adduser

sensor_adduser_LDFLAGS = -export-dynamic
sensor_adduser_LDADD = $(top_srcdir)/src/libprelude.la -lssl -ldl -lpthread


sensor_adduser_SOURCES = 		\
	sensor-adduser.c		\
	ssl-register.c			\
	plaintext.c


noinst_HEADERS = plaintext.h ssl-register.h
