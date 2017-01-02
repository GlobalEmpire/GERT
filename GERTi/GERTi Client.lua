-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local m = component.modem
local neighbors = {}
local neighborDex = 1

local function storeNeighbors(eventName, receivingModem, sendingModem, port, distance, package)
    neighbors[neighborDex]["address"] = sendingModem
    neighbors[neighborDex]["tier"] = tonumber(package)
    neighborDex = neighborDex + 1
end
-- startup procedure
m.open(4378)
if modem.isWireless() then
    modem.setStrength(500)
end
m.broadcast(4378, "GERTiStart")
while true do
    storeNeighbors(event.pull(1, "modem_message"))
end

local function sendPacket(_, finalAddress, packet)
    --do some stuff here to send the packet on to final address
end

local function processPacket(_, packet)
    packet = serialize.unserialize(packet)
    --local destination
    -- do some stuff here to process the packet and prepare for sendout
end

local function receivePacket(_, packet)
    -- do some stuff here to receive the packet and get it ready for processing
end

event.listen("modem_message", receivePacket)
event.listen("GERTiTransmit", processPacket)