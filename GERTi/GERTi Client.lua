-- Under Construction
local GERTi = {}
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local modem = component.modem
if component.isAvailable("tunnel") then
    local tunnel = component.tunnel
end
local tier = 3
local neighbors = {}
local neighborDex = 1
local serialTable = ""

-- functions for startup
local function sortTable(elementOne, elementTwo)
    if tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]) then
        return true
    else
        return false
    end
end
local function storeNeighbors(eventName, receivingModem, sendingModem, port, distance, package)
    -- register neighbors for communication to gateway
    neighbors[neighborDex] = {}
    neighbors[neighborDex]["address"] = sendingModem
    neighbors[neighborDex]["port"] = tonumber(port)
    if package == nil then
        neighbors[neighborDex]["tier"] = (tier+1)
    else
        neighbors[neighborDex]["tier"] = tonumber(package)
        if tonumber(package) < tier then
            tier = tonumber(package)+1
        end
    end
    neighborDex = neighborDex + 1
    -- sort table so that the best connection to the gateway comes first
    table.sort(neighbors, sortTable)
end
local function handleEvent(...)
    if ... ~= nil then
        storeNeighbors(...)
        return "not nil"
    else
        return "nillerino"
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
    print(isNil)
    if isNil == "nillerino" then
        break
    end
end

-- functions for normal operation
local function transmitInformation(sendTo, port, ...)
    if port ~= 0 then
        modem.send(sendTo, port, ...)
    else
        tunnel.send(...)
    end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, ...)
    print(...)
    -- filter out neighbor requests, and send on packet for further processing
    if (...) == "GERTiStart" and tier < 3 then
        storeNeighbors(eventName, receivingModem, sendingModem, port, distance, nil)
        transmitInformation(sendingModem, port, tier)
    elseif (...) == "GERTiForwardTable" then
        transmitInformation(neighbors[1]["address"], neighbors[1]["port"], ...)
    end
end
-- engage listener so that other computers can connect
event.listen("modem_message", receivePacket)
-- forward neighbor table up the line
serialTable = serialize.serialize(neighbors)
print(serialTable)
if serialTable ~= "{}" then
    transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "GERTiForwardTable", modem.address, tier, serialTable)
end
-- startup procedure is now complete
-- begin procedure to allow for data transmission
-- this function allows a connection to the requested destination device
function GERTi.openRoute(destination)
    
end
-- this function will allow a program to receive data from a GERTi connection
function GERTi.receiveData()
end
-- There'll be some stuff here to allow a program to create an object they can read/write data from, similar to the internet card's internet.socket().