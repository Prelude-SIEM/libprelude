if HAVE_PYTHON2

EXTRA_DIST = libprelude-python.i prelude.py _prelude.c

all-am: python-build

python-build: _prelude.c
	CC="$(BINDINGS_CC)" $(PYTHON2) setup.py build

install-exec-hook:
	$(PYTHON2) setup.py install `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`

uninstall-hook: 
	$(PYTHON2) setup.py uninstall `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`

clean-local:
	$(PYTHON2) setup.py clean -a

_prelude.c: $(top_srcdir)/bindings/low-level/libprelude.i libprelude-python.i
if HAVE_SWIG
	$(SWIG) -I$(top_srcdir)/bindings -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -o $@ -python -noproxy -interface _prelude -module _prelude $(top_srcdir)/bindings/low-level/libprelude.i
endif

endif

-include $(top_srcdir)/git.mk
