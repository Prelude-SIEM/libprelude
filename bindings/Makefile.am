EXTRA_DIST = libprelude.i perl/Makefile.PL perl/Prelude.pm python/setup.py python/prelude.py

all-am: perl python



perl: perl/Prelude.c perl/Makefile
	cd perl && make

perl/Prelude.c: libprelude.i
	swig -I$(top_srcdir)/src/include -o $@ -perl5 -nopm $<

perl/Makefile: perl/Makefile.PL
	cd perl && perl Makefile.PL



python: python/_prelude.c
	cd python && python setup.py build

python/_prelude.c: libprelude.i
	swig -I$(top_srcdir)/src/include -o $@ -python -interface _prelude -module _prelude -noproxy $<



install-exec-hook:
	cd perl && make install
	cd python && python setup.py install

clean-am: perl/Makefile
	cd perl && make clean
	cd python && python setup.py clean
