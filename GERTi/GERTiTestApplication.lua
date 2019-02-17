-- Tested with GERTi v1.1
-- This provides a very simple chat program between 3 GERTi Clients to demonstrate network functionality. If communication between only two computers is desired, please comment out lines 8, 11, 24-25, 31, and 35. 
-- The GERTi addresses must be configured manually as this is a very simple program. The UI is terrible, but a better chat program can be found at https://github.com/GlobalEmpire/OC-Programs/tree/master/Programs/OpenChat
local event = require("event")
local GERTi = require("GERTiClient")
local term = require("term")
local socket1 = GERTi.openSocket(0.2, true, 1)
local socket2 = GERTi.openSocket(0.3, true, 1)
local function incomingConnection()
	socket1:read()
	socket2:read()
end
event.listen("GERTConnectionID", incomingConnection)

local function messageOutput(origin, data)
	for key, value in pairs(data) do
		term.write(origin.." says: "..value.."\n")
	end
end

local function messageReceived(eventName, origination, connectionID)
	if tostring(origination)=="0.2" then
		messageOutput("GERTBooth2", socket1:read())
	elseif tostring(origination) == "0.3" then
		messageOutput("GERTBooth3", socket2:read())
	end
end

event.listen("GERTData", messageReceived)
messageReceived(nil, "0.2", 1)
messageReceived(nil, "0.3", 1)
while true do
	local input = term.read()
	socket1:write(input)
	socket2:write(input)
	term.write("\n")
end