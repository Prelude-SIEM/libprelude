#!/usr/bin/env lua

require("prelude")

function my_cb(level, log)
	io.write("log: " .. log)
end
prelude.PreludeLog_setCallback(my_cb)


function tprint (tbl, indent)
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      tprint(v, indent+1)
    elseif not v then
      print(formatting .. 'NUL')
    elseif type(v) == 'boolean' then
      print(formatting .. tostring(v))
    else
      print(formatting .. tostring(v))
    end
  end
end


idmef = prelude.IDMEF()

print("*** IDMEF->Set() ***")
idmef:set("alert.classification.text", "My Message")
idmef:set("alert.source(0).node.address(0).address", "s0a0")
idmef:set("alert.source(0).node.address(1).address", "s0a1")
idmef:set("alert.source(1).node.address(0).address", "s1a0")
idmef:set("alert.source(1).node.address(1).address", "s1a1")
idmef:set("alert.source(1).node.address(2).address", nil)
idmef:set("alert.source(1).node.address(3).address", "s1a3")
print(idmef)

print("\n*** Value IDMEF->Get() ***")
print(idmef:get("alert.classification.text"))

print("\n*** Listed Value IDMEF->Get() ***")
tprint(idmef:get("alert.source(*).node.address(*).address"))

print("\n*** Object IDMEF->Get() ***")
print(idmef:get("alert.source(0).node.address(0)"))

print("\n*** Listed Object IDMEF->Get() ***")
tprint(idmef:get("alert.source(*).node.address(*)"))


fd = io.open("foo.bin","w")
idmef:write(fd)
fd:close()

print("\n*** IDMEF->Read() ***")
fd2 = io.open("foo.bin","r")
idmef2 = prelude.IDMEF()
while idmef2:read(fd2) do
	print(idmef2)
end
fd2:close()


print("\n*** Client ***")
c = prelude.ClientEasy("prelude-lml")
c:start()

c:sendIDMEF(idmef)
