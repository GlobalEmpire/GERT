-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local modem = component.modem
local tunnel = component.tunnel

local tier = 100
local neighbors = {}
local neighborDex = 1

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
local function transmitInformation(sendTo)
    modem.send(sendTo, 4378, tier)
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
    
end

event.listen("modem_message", receivePacket)