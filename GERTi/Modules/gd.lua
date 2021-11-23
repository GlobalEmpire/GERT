local g = require("GERTiClient")
local s = require("shell")

local a,n = s.parse(...)
arg = a[1]

local function Help()
  print("GERTi Diagnostics Program Documentation")
  print(" ")
  print("gd help - print this documentation")
  print("gd neigh - get network neighbors")
  print("gd ping [address] - attempt to ping [address]")
  print("gd addr - get this computer's GERT address")
end

if not arg then
  Help()
elseif arg == "help" then
  Help()
elseif arg == "neigh" then
  local neigh = g.getNeighbors()
  if neigh then
    local keyset={}
    local n=0
    
    for k,v in pairs(neigh) do
      n=n+1
      keyset[n]=k
    end
    table.sort(keyset)
    print("Your Neighbors Are:")
    for i,t in ipairs(keyset) do
      print(t)
    end
  else
    print("No Neighbors Found")
  end
elseif arg == "ping" then
  socket = g.openSocket(a[2])
  if socket then
    print("Ping Successful")
    socket:close()
  else
    print("Ping Failed")
  end
elseif arg == "addr" then
  print("Your GERT Address is: " .. g.getAddress())
end
