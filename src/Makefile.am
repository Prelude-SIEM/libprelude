SUBDIRS = include
DEFS = -DCONFIG_DIR=\"@configdir@\"

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libmissing -I$(srcdir)/include 

libpreludeincludedir = $(includedir)/
lib_LTLIBRARIES = libprelude.la

libprelude_la_SOURCES = 		\
	config-engine.c 		\
	auth-common.c			\
	plugin-common.c			\
	daemonize.c			\
	ssl-registration-msg.c 		\
	ssl-gencrypto.c			\
	ssl-config.c			\
	socket-op.c			\
	string-matching.c		\
	timer.c				\
	alert-common.c

libprelude_la_LIBADD = ../libmissing/libmissing.a
