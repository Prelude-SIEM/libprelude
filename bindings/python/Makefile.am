if HAVE_EASY_BINDINGS
if HAVE_PYTHON

EXTRA_DIST = libpreludecpp-python.i PreludeEasy.py _PreludeEasy.cxx

all-am: python-build

python-build: _PreludeEasy.cxx
	CC="$(CXX)" $(PYTHON) setup.py build

install-exec-hook:
	$(PYTHON) setup.py install `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`

uninstall-hook:
	$(PYTHON) setup.py uninstall `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`

clean-local:
	$(PYTHON) setup.py clean -a


_PreludeEasy.cxx PreludeEasy.py: $(top_srcdir)/bindings/c++/include/*.hxx $(top_srcdir)/bindings/libpreludecpp.i libpreludecpp-python.i 
if HAVE_SWIG
	$(SWIG) -c++ -I$(top_srcdir)/bindings -I$(top_srcdir)/bindings/c++/include -I$(top_srcdir)/src/include -I$(top_srcdir)/src/libprelude-error -python -o $@ ../libpreludecpp.i
endif

endif
endif

-include $(top_srcdir)/git.mk
