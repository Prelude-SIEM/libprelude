BINDING_DEPENDS=libprelude.i libprelude_python.i libprelude_perl.i $(top_srcdir)/src/include/prelude-client.h $(top_srcdir)/src/include/idmef-tree-wrap.h $(top_srcdir)/src/include/idmef-value.h $(top_srcdir)/src/include/idmef-path.h $(top_srcdir)/src/include/idmef-time.h $(top_srcdir)/src/include/idmef-data.h $(top_srcdir)/src/include/prelude-string.h $(top_srcdir)/src/include/idmef-criteria.h $(top_srcdir)/src/include/prelude-msgbuf.h $(top_srcdir)/src/include/idmef-message-write.h $(top_srcdir)/src/include/idmef-message-print.h $(top_srcdir)/src/include/idmef-value-type.h $(top_srcdir)/src/include/idmef-type.h $(top_srcdir)/src/include/prelude-connection.h $(top_srcdir)/src/include/prelude-option.h $(top_srcdir)/src/include/prelude-option-wide.h $(top_srcdir)/src/include/prelude-message-id.h $(top_srcdir)/src/include/prelude-log.h

EXTRA_DIST = libprelude.i perl/Makefile.PL.in perl/Prelude.pm perl/Prelude.c python/setup.py.in \
	python/prelude.py python/_prelude.c

all-am: perl python

if HAVE_PERL
perl: perl/Prelude.c perl/Makefile perl-build
else
perl:
endif


if HAVE_PYTHON
python: python/_prelude.c python-build
else
python:
endif


if HAVE_PERL
perl/Makefile: perl/Makefile.PL
	cd perl && DESTDIR=$(DESTDIR) $(PERL) Makefile.PL
endif


# proxy classes are an option for swig <= 1.3.19 and are activated by default with swig > 1.3.19
# as we don't want proxy classes, we must explicitly tell swig > 1.3.19 (that support the -noproxy option)
# to not generate them
perl/Prelude.c: $(BINDING_DEPENDS)
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -o $@ -perl5 -nopm \
	`$(SWIG) -perl5 -help 2>&1 | grep noproxy > /dev/null && echo -noproxy` $<
endif

python/_prelude.c: $(BINDING_DEPENDS)
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -o $@ -python -noproxy -interface _prelude -module _prelude $<
endif


perl-build:
	cd perl && $(MAKE) CC=$(CC)

python-build:
	cd python && $(PYTHON) setup.py build



install-exec-hook:
if HAVE_PERL
	cd perl && DESTDIR=$(DESTDIR) $(PERL) Makefile.PL && $(MAKE) install
endif

if HAVE_PYTHON
	cd python && $(PYTHON) setup.py install `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`
endif

uninstall-hook:
if HAVE_PERL
	cd perl && DESTDIR=$(DESTDIR) prefix=$(prefix) $(PERL) Makefile.PL && $(MAKE) uninstall
endif

if HAVE_PYTHON
	cd python && python setup.py uninstall `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`
endif


clean-am:
if HAVE_PERL
	cd perl && $(MAKE) clean
endif

if HAVE_PYTHON
	cd python && $(PYTHON) setup.py clean -a
endif
