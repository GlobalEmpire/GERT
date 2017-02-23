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
    isNil = handleEvent(event.pull(2, "modem_message"))
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
    -- process packet according to what the identifier string is, and potentially perform further processing
    if (...) == "OPENROUTE" then
        local message, destination, intermediary, origination = ...
        -- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
        if destination == modem.address then
            transmitInformation(sendingModem, port, "ROUTE OPEN")
            print("opening route")
        else
            -- attempt to check if destination is a neighbor to this computer, if so, re-transmit OPENROUTE message to the neighbor so routing can be completed
            local isNeighbor = false
            for key, value in pairs(neighbors) do
                if value["address"] == destination then
                    isNeighbor = true
                    -- transmit OPENROUTE and then wait to see if ROUTE OPEN response is acquired
                    transmitInformation(neighbors[key]["address"], neighbors[key]["port"], "OPENROUTE", destination)
                    local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                    if payload == "ROUTE OPEN" then
                        transmitInformation(sendingModem, port, "ROUTE OPEN")
                    end
                    break
                end
            end
            if isNeighbor == false and intermediary == nil then
                -- if it is not a neighbor, then contact parent to forward indirect connection request
                transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "OPENROUTE", destination, nil, origination)
                local eventName, receivingModem, _, port, distance, payload = event.pull(8, "modem_message")
                if payload == "ROUTE OPEN" then
                    transmitInformation(sendingModem, port, "ROUTE OPEN")
                end
                -- if it is not a neighbor, but an intermediary is found, then attempt to forward request to intermediary
            elseif isNeighbor == false then
                for key, value in pairs(neighbors) do
                    if value["address"] == intermediary then
                        transmitInformation(neighbors[key]["address"], neighbors[key]["port"], "OPENROUTE", destination, intermediary, origination)
                        local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                        if payload == "ROUTE OPEN" then
                            transmitInformation(sendingModem, port, "ROUTE OPEN")
                        end
                        break
                    end
                end
            end
        end
    elseif(...) == "GERTiStart" and tier < 3 then
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
-- this function allows a connection to the requested destination device, should only be used publicly if low-level operation is desired (e.g. another protocol that sits on top of GERTi to further manage networking)
function GERTi.openRoute(destination)
    local isNeighbor = false
    local neighborKey = 0
    local isOpen = false
    -- attempt to see if neighbor is local
    for key, value in pairs(neighbors) do 
        if value["address"] == destination then
            isNeighbor = true
            neighborKey = key
            break
        end
    end
    -- if neighbor is local, then open a direct connection
    if isNeighbor == true then
        transmitInformation(neighbors[neighborKey]["address"], neighbors[neighborKey]["port"], "OPENROUTE", destination)
        local eventName, receivingModem, sendingModem, port, distance, payload = event.pull(2, "modem_message")
        print(payload)
        if payload == "ROUTE OPEN" then
            isOpen = true
        end
        print(isOpen)
        return isOpen
    else
        -- if neighbor is not local, then attempt to contact parent to open an indirect connection (i.e. routed through multiple computers)
        transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "OPENROUTE", destination, nil, modem.address)
        -- timeout of 8 seconds chosen to ensure that routing request will go through, even if all the way across network (tier 3->2->1->gateway->1->2->3)
        local eventName, receivingModem, sendingModem, port, distance, payload = event.pull(8, "modem_message")
        if payload == "ROUTE OPEN" then
            isOpen = true
        end
        print(isOpen)
        return isOpen
    end
end
-- There'll be some stuff here to allow a program to create an object they can read/write data from, similar to the internet card's internet.socket().