BINDING_DEPENDS=libprelude.i ../src/include/prelude-client.h ../src/include/idmef-tree-wrap.h ../src/include/idmef-value.h ../src/include/idmef-object.h ../src/include/idmef-time.h ../src/include/idmef-data.h ../src/include/idmef-string.h ../src/include/idmef-criteria.h ../src/include/idmef-criteria-string.h ../src/include/idmef-object-value.h ../src/include/prelude-message-buffered.h ../src/include/idmef-message-write.h ../src/include/idmef-tree-print.h ../src/include/idmef-tree-to-string.h ../src/include/idmef-value-type.h ../src/include/idmef-util.h ../src/include/idmef-type.h ../src/include/prelude-connection.h ../src/include/prelude-getopt.h ../src/include/prelude-getopt-wide.h ../src/include/prelude-message-id.h ../src/include/prelude-log.h

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



perl/Makefile: perl/Makefile.PL
	cd perl && DESTDIR=$(DESTDIR) $(PERL) Makefile.PL


# proxy classes are an option for swig <= 1.3.19 and are activated by default with swig > 1.3.19
# as we don't want proxy classes, we must explicitly tell swig > 1.3.19 (that support the -noproxy option)
# to not generate them
perl/Prelude.c: $(BINDING_DEPENDS)
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/src/include -o $@ -perl5 -nopm \
	 `$(SWIG) -perl5 -help 2>&1 | grep noproxy > /dev/null && echo -noproxy` $<
endif

python/_prelude.c: $(BINDING_DEPENDS)
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/src/include -o $@ -python -interface _prelude -module _prelude -noproxy $<
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


clean-am: perl/Makefile
if HAVE_PERL
	cd perl && $(MAKE) clean
endif

if HAVE_PYTHON
	cd python && $(PYTHON) setup.py clean -a
endif
