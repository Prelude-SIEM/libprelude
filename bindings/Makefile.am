EXTRA_DIST = libprelude.i perl/Makefile.PL perl/Prelude.pm perl/Prelude.c python/setup.py python/prelude.py python/_prelude.c

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
	cd perl && perl Makefile.PL

perl/Prelude.c: libprelude.i
if HAVE_SWIG
	swig -I$(top_srcdir)/src/include -o $@ -perl5 -nopm $<
endif

python/_prelude.c: libprelude.i
if HAVE_SWIG
	swig -I$(top_srcdir)/src/include -o $@ -python -interface _prelude -module _prelude -noproxy $<
endif


perl-build:
	cd perl && $(MAKE)

python-build:
	cd python && python setup.py build



install-exec-hook:
if HAVE_PERL
	cd perl && $(MAKE) install
endif

if HAVE_PYTHON
	cd python && python setup.py install
endif



clean-am: perl/Makefile
if HAVE_PERL
	cd perl && $(MAKE) clean
endif

if HAVE_PYTHON
	cd python && python setup.py clean -a
endif