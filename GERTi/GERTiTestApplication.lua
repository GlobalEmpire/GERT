-- This application is for use testing GERTi.
-- It accepts a limited set of simple, short commands.
-- It does not represent an actual usecase of GERTi.
--  - 20kdc

local C = require("GERTiClient")
local socket = nil
while true do
 local s = io.read()
 if s == "address" then
  print("My address is:", C.getAddress())
 end
 if s == "open" then
  print("Target?")
  local skt, err = C.openSocket(io.read())
  print("~~ RES:", skt, err)
  socket = skt
 end
 if s == "write" then
  if math.random() > 0.5 then
   socket:write("The big red dog jumped over the lazy cat.")
  else
   socket:write("My house is full of bicycles.")
  end
 end
 if s == "read" then
  print(table.unpack(socket:read()))
 end
end
