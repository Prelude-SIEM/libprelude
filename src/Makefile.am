SUBDIRS = include
DEFS = @defined@ -DCONFIG_DIR=\"@configdir@\" -DLOCALSTATEDIR=\"@localstatedir@\"
INCLUDES = -I$(top_srcdir)/include -I$(srcdir)/include

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
	compat.c			\
	string-matching.c		\
	timer.c				\
	alert-common.c





