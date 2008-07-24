#!/usr/bin/env python

import PreludeEasy

idmef = PreludeEasy.IDMEF()

print "*** IDMEF->Set() ***"
idmef.Set("alert.classification.text", "My Message")
idmef.Set("alert.source(0).node.address(0).address", "x.x.x.x")
idmef.Set("alert.source(0).node.address(1).address", "y.y.y.y")
idmef.Set("alert.target(0).node.address(0).address", "z.z.z.z")
print idmef


print "\n*** IDMEF->Get() ***"
print idmef.Get("alert.classification.text")

def print_list(x):
   for i in x:
       if type(i) is tuple:
	   print_list(i)
       else:
	   print i

print_list(idmef.Get("alert.source(*).node.address(*).address"))

file = open("foo.bin","w")
idmef >> file
#idmef.Write(file);
file.close()

file2 = open("foo.bin","r")
idmef2 = PreludeEasy.IDMEF()
idmef2 << file2
#idmef2.Read(file2);
file2.close()
print idmef2


print "\n*** Client ***"
c = PreludeEasy.ClientEasy("prelude-lml")
c.Start()

c << idmef
