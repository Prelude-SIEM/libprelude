#!/usr/bin/env ruby 

require("PreludeEasy")

PreludeEasy::PreludeLog::SetCallback(lambda{|level,str|print "log: " + str})
idmef = PreludeEasy::IDMEF.new()

print "*** IDMEF->Set() ***\n"
idmef.Set("alert.classification.text", "My Message")
idmef.Set("alert.source(0).node.address(0).address", "x.x.x.x")
idmef.Set("alert.source(0).node.address(1).address", "y.y.y.y")
idmef.Set("alert.target(0).node.address(0).address", "z.z.z.z")
print idmef


print "\n*** IDMEF->Get() ***\n"
print idmef.Get("alert.classification.text") + "\n"

def print_list(x)
   for i in x
       if i.class() == Array
	   print_list(i)
       else
	   print i + "\n";
       end
   end
end

print_list(idmef.Get("alert.source(*).node.address(*).address"))

fd = File.new("foo.bin", "w")
idmef >> fd
#idmef.Write(fd)
fd.close()

fd2 = File.new("foo.bin", "r")
idmef2 = PreludeEasy::IDMEF.new()
idmef2 << fd2;
#idmef2.Read(fd2)
fd2.close()
print idmef2

print "\n*** Client ***\n"
c = PreludeEasy::ClientEasy.new("prelude-lml")
c.Start()

c << idmef
