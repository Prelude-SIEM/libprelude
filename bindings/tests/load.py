#!/usr/bin/python

import sys
sys.path.append('.')
sys.path.append('./.libs')

try:
	import PreludeEasy
except Exception,e:
	print "Import failed: ",e
	print "Try 'cd ./.libs && ln -s libprelude_python.so _PreludeEasy.so'"
	sys.exit(1)

idmef = PreludeEasy.IDMEF()
idmef.ReadFromFile("foo.bin")
idmef.PrintToStdout()
