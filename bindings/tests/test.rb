#!/usr/bin/env ruby 

require("Prelude")

Prelude::PreludeLog::setCallback(lambda{|level,str|print "log: " + str})
idmef = Prelude::IDMEF.new()

print "*** IDMEF->Set() ***\n"
idmef.set("alert.classification.text", "My Message")
idmef.set("alert.source(0).node.address(0).address", "s0a1")
idmef.set("alert.source(0).node.address(1).address", "s0a2")
idmef.set("alert.source(1).node.address(0).address", "s1a1")
idmef.set("alert.source(1).node.address(1).address", "s1a2")
idmef.set("alert.source(1).node.address(2).address", nil)
idmef.set("alert.source(1).node.address(3).address", "s1a3")
print idmef

print "\n*** Value IDMEF->Get() ***\n"
print idmef.get("alert.classification.text")

print "\n\n*** Listed Value IDMEF->Get() ***\n"
print idmef.get("alert.source(*).node.address(*).address")

print "\n\n*** Object IDMEF->Get() ***\n"
print idmef.get("alert.source(0).node.address(0)")

print "\n\n*** Listed Object IDMEF->Get() ***\n"
print idmef.get("alert.source(*).node.address(*)")
print "\n\n"

fd = File.new("foo.bin", "w")
idmef >> fd
#idmef.Write(fd)
fd.close()

print "\n*** IDMEF->Read() ***\n"
fd2 = File.new("foo.bin", "r")
idmef2 = Prelude::IDMEF.new()
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
idmef2 = Prelude::IDMEF.new()
while idmef2.read(fd2) > 0 do
	print idmef2
end
fd2.close()

print "\n*** Client ***\n"
c = Prelude::ClientEasy.new("prelude-lml")
c.start()

c << idmef
