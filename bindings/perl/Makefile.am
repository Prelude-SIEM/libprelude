###
# We don't use libtool to compile Prelude.so because libtool --mode=install does not
# allow us to install Prelude.so in a directory not in ${prefix}/*
###

DEFS=-I$(top_srcdir) -I$(top_srcdir)/src/include -D_GNU_SOURCE @PERL_CFLAGS@
CFLAGS=
LINK = $(CC) -o $@

if HAVE_PERL

bin_PROGRAMS = Prelude.so

Prelude_so_LDFLAGS = -shared -L$(top_srcdir)/src/.libs
Prelude_so_SOURCES = Prelude.c
Prelude_so_LDADD = @libprelude_libs@ -lprelude
noinst_PROGRAMS = Prelude.so

endif

install-exec-local:
if HAVE_PERL
	$(INSTALL) -d -m 755 $(DESTDIR)/$(perl_dir)
	$(INSTALL) -m 644 Prelude.pm $(DESTDIR)/$(perl_dir)
	$(INSTALL) -d -m 755 $(DESTDIR)/$(perl_dir)/auto/Prelude
	$(INSTALL) -m 644 Prelude.so $(DESTDIR)/$(perl_dir)/auto/Prelude
endif
