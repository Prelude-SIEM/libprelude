#!/usr/bin/python

import sys
sys.path.append('.')
sys.path.append('./.libs')

try:
	import PreludeEasy
except:
	print "Import failed"
	print "Try 'cd ./.libs && ln -s libprelude_python.so _PreludeEasy.so'"
	sys.exit(1)

def foo(id):
        print "callback: id = " + str(id)
	idmef = PreludeEasy._get_IDMEF(id)
        idmef.PrintToStdout()
        #print bar.Get("alert.classification.text") # XXX not yet implemented
        return 0

PreludeEasy.set_pymethod(foo)

PreludeEasy.test_fct()
