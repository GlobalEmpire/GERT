-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local modem = component.modem
local tunnel = component.tunnel

local tier = 100
local neighbors = {}
local neighborDex = 1
local serialTable = ""

-- functions for startup
local function handleEvent(...)
    if eventName ~= nil then
        storeNeighbors(...)
    else
        return "nillerino"
    end
end
local function storeNeighbors(eventName, receivingModem, sendingModem, port, distance, package)
    -- register neighbors for communication to gateway
    neighbors[neighborDex]["address"] = sendingModem
    neighbors[neighborDex]["tier"] = tonumber(package)
    neighbors[neighborDex]["port"] = tonumber(port)
    if tonumber(package) < tier then
        tier = tonumber(package) + 1
    end
    neighborDex = neighborDex + 1
end
local function sortTable(elementOne, elementTwo)
    if tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"])
        return true
    else
        return false
    end
end
-- startup procedure
-- check if linked card is available and then attempt to s
if component.isAvailable("tunnel") then
    tunnel.send("GERTiStart")
    handleEvent(event.pull(1, "modem_message"))
end
-- now attempt to use a network card for finding neighbors
-- open port and prepare for transmission of message
modem.open(4378)
if modem.isWireless() then
    modem.setStrength(500)
end
-- transmit broadcast to check for neighboring GERTi enabled computers
modem.broadcast(4378, "GERTiStart")
while true do
    local isNil = "not nil"
    isNil = handleEvent(event.pull(1, "modem_message"))
    if isNil == "nillerino" then
        print(isNil)
        break
    end
end

-- sort table so that the lowest tier connections come first
table.sort(neighbors, sortTable)

-- functions for normal operation
local function transmitInformation(sendTo, port, ...)
    if port ~= 0 then
        modem.send(sendTo, port, ...)
    else
        tunnel.send(...)
    end
end
local function sendPacket(_, finalAddress, packet)
    --do some stuff here to send the packet on to final address
end

local function processPacket(_, packet)
    packet = serialize.unserialize(packet)
    --local destination
    -- do some stuff here to process the packet and prepare for sendout
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, ...)
    -- filter out neighbor requests, and send on packet for further processing
    if ... == "GERTiStart" and tier < 3 then
        if port ~= 0 then
            transmitInformation(sendingModem, 4378, tier)
        else
            transmitInformation(sendingModem, 0)
        end
    elseif ... == "GERTiForwardTable"
        transmitInformation(neighbors[neighborDex][1]["address"], 4378, tier)
    end
end
-- engage listener so that other computers can connect
event.listen("modem_message", receivePacket)
-- forward neighbor table up the line
serialTable = serialize.serialize(neighbors)
transmitInformation(neighbors[1]["address"], 4378, "GERTiForwardTable", serialTable)