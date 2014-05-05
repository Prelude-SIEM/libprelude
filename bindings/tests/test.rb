#!/usr/bin/env ruby 

require("PreludeEasy")

PreludeEasy::PreludeLog::SetCallback(lambda{|level,str|print "log: " + str})
idmef = PreludeEasy::IDMEF.new()

print "*** IDMEF->Set() ***\n"
idmef.Set("alert.classification.text", "My Message")
idmef.Set("alert.source(0).node.address(0).address", "s0a1")
idmef.Set("alert.source(0).node.address(1).address", "s0a2")
idmef.Set("alert.source(1).node.address(0).address", "s1a1")
idmef.Set("alert.source(1).node.address(1).address", "s1a2")
idmef.Set("alert.source(1).node.address(2).address", nil)
idmef.Set("alert.source(1).node.address(3).address", "s1a3")
print idmef


print "\n*** IDMEF->Get() ***\n"
print idmef.Get("alert.classification.text") + "\n"
print(idmef.Get("alert.source(*).node.address(*).address"))

fd = File.new("foo.bin", "w")
idmef >> fd
#idmef.Write(fd)
fd.close()

print "\n*** IDMEF->Read() ***\n"
fd2 = File.new("foo.bin", "r")
idmef2 = PreludeEasy::IDMEF.new()
while true do
	begin
		idmef2 << fd2
		print idmef2
	rescue EOFError
                print "Got EOF\n"
		break
	end
end
fd2.close()

fd2 = File.new("foo.bin", "r")
idmef2 = PreludeEasy::IDMEF.new()
while idmef2.Read(fd2) > 0 do
	print idmef2
end
fd2.close()

print "\n*** Client ***\n"
c = PreludeEasy::ClientEasy.new("prelude-lml")
c.Start()

c << idmef
