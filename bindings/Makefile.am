EXTRA_DIST = libprelude.i perl/Makefile.PL.in perl/Prelude.pm perl/Prelude.c python/setup.py.in	 python/prelude.py python/_prelude.c

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


# proxy classes are an option for swig <= 1.3.19 and are activated by default with swig > 1.3.19
# as we don't want proxy classes, we must explicitly tell swig > 1.3.19 (that support the -noproxy option)
# to not generate them
perl/Prelude.c: libprelude.i
if HAVE_SWIG
	swig -I$(top_srcdir)/src/include -o $@ -perl5 -nopm \
	 `swig -perl5 -help 2>&1 | grep noproxy > /dev/null && echo -noproxy` $<
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