-- Under Construction
local GERTi = {}
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local modem = component.modem
if component.isAvailable("tunnel") then
    local tunnel = component.tunnel
end
local neighbors = {}
local tier = 3
local neighborDex = 1
-- table of open connections
local connections = {}
local connectDex = 1

-- internal functions
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

local function storeConnection(origination, destination, beforeHop, nextHop, port)
    connections[connectDex] = {}
    connections[connectDex]["destination"] = destination
    connections[connectDex]["origination"] = origination
    connections[connectDex]["beforeHop"] = beforeHop
    connections[connectDex]["nextHop"] = nextHop
    connections[connectDex]["port"] = port
    connections[connectDex]["data"] = {}
    connections[connectDex]["dataDex"] = 1
    connectDex = connectDex + 1
    return (connectDex-1)
end

local function storeConnections(origination, destination, beforeHop, nextHop, port)
    storeConnection(origination, destination, beforeHop, nextHop, port)
    storeConnection(destination, origination, nextHop, beforeHop, port)
end

local function storeData(connectNum, data)
    local dataNum = connections[connectNum]["dataDex"]
    if dataNum < 20 then
        connections[connectNum]["data"][dataNum]=data
        connections[connectNum]["dataDex"] = dataNum + 1
    else
        connections[connectNum]["dataDex"] = 1
        storeData(connectNum, data)
    end
end

local function transmitInformation(sendTo, port, ...)
    if port ~= 0 then
        modem.send(sendTo, port, ...)
    else
        tunnel.send(...)
    end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
    print(code)
    -- process packet according to what the identifier string is, and potentially perform further processing
    if (code) == "DATA" then
        local data, destination, origination = ...
        local connectNum = 0
        for key, value in pairs(connections) do
            if value["destination"] == destination and value["origination"] == origination then
                connectNum = key
                break
            end
        end
        if connectNum ~= 0 then
            -- connectNum should never ever be 0, but I don't even know these days.
            if connections[connectNum]["destination"] == modem.address then
                storeData(connectNum, data)    
            else
                transmitInformation(connections[connectNum]["nextHop"], connections[connectNum]["port"], "DATA", data, destination, origination)
            end
        end     
        
    elseif (code) == "OPENROUTE" then
        local destination, intermediary, intermediary2, origination = ...
        -- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
        if destination == modem.address then
            transmitInformation(sendingModem, port, "ROUTE OPEN")
            if intermediary ~= nil then
                storeConnections(origination, modem.address, intermediary, modem.address, port)
            else
                storeConnections(modem.address, origination, sendingModem, modem.address, port)
            end
            print("opening route")
            
        else
            -- attempt to check if destination is a neighbor to this computer, if so, re-transmit OPENROUTE message to the neighbor so routing can be completed
            local isNeighbor = false
            for key, value in pairs(neighbors) do
                if value["address"] == destination then
                    isNeighbor = true
                    -- transmit OPENROUTE and then wait to see if ROUTE OPEN response is acquired
                    transmitInformation(neighbors[key]["address"], neighbors[key]["port"], "OPENROUTE", destination, modem.address, nil, origination)
                    local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                    if payload == "ROUTE OPEN" then
                        transmitInformation(sendingModem, port, "ROUTE OPEN")
                        storeConnections(destination, origination, sendingModem, neighbors[key]["address"], port)
                    end
                    break
                end
            end
            
            if isNeighbor == false and intermediary2 == nil then
                -- if it is not a neighbor, then contact parent to forward indirect connection request
                transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "OPENROUTE", destination, modem.address, nil, origination)
                local eventName, receivingModem, _, port, distance, payload = event.pull(8, "modem_message")
                if payload == "ROUTE OPEN" then
                    transmitInformation(sendingModem, port, "ROUTE OPEN")
                    storeConnections(destination, origination, sendingModem, neighbors[1]["address"], neighbors[1]["port"])
                end
                -- if it is not a neighbor, but an intermediary is found, then attempt to forward request to intermediary
            elseif isNeighbor == false then
                for key, value in pairs(neighbors) do
                    if value["address"] == intermediary2 then
                        transmitInformation(neighbors[key]["address"], neighbors[key]["port"], "OPENROUTE", destination, modem.address, nil, origination)
                        local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                        if payload == "ROUTE OPEN" then
                            transmitInformation(sendingModem, port, "ROUTE OPEN")
                            storeConnections(destination, origination, sendingModem, intermediary2, neighbors[key]["port"])
                        end
                        break
                    end
                end
            end
        end
        
    elseif(code) == "GERTiStart" and tier < 3 then
        storeNeighbors(eventName, receivingModem, sendingModem, port, distance, nil)
        transmitInformation(sendingModem, port, tier)
        
    elseif (code) == "GERTiForwardTable" then
        transmitInformation(neighbors[1]["address"], neighbors[1]["port"], ...)
    end
end

local function handleEvent(...)
    if ... ~= nil then
        storeNeighbors(...)
        return true
    else
        return false
    end
end

-- transmit broadcast to check for neighboring GERTi enabled computers
if component.isAvailable("tunnel") then
    tunnel.send("GERTiStart")
    handleEvent(event.pull(1, "modem_message"))
end

modem.open(4378)
if modem.isWireless() then
    modem.setStrength(500)
end

modem.broadcast(4378, "GERTiStart")
local continue = true
while continue do
    continue = handleEvent(event.pull(2, "modem_message"))
end
event.listen("modem_message", receivePacket)

-- forward neighbor table up the line
local serialTable = ""
serialTable = serialize.serialize(neighbors)
print(serialTable)
if serialTable ~= "{}" then
    transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "GERTiForwardTable", modem.address, tier, serialTable)
end
-- startup procedure is now complete
-- begin procedure to allow for data transmission
-- this function allows a connection to the requested destination device, should only be used publicly if low-level operation is desired (e.g. another protocol that sits on top of GERTi to further manage networking)
function GERTi.openRoute(destination)
    local connectNum = 0
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
        transmitInformation(neighbors[neighborKey]["address"], neighbors[neighborKey]["port"], "OPENROUTE", destination, nil, nil, modem.address)
        local eventName, receivingModem, sendingModem, port, distance, payload = event.pull(2, "modem_message")
        print(payload)
        if payload == "ROUTE OPEN" then
            isOpen = true
        end
        print(isOpen)
        if isOpen == true then
            connectNum = storeConnections(destination, modem.address, modem.address, destination, port)
        end
        return isOpen, connectNum
    else
        -- if neighbor is not local, then attempt to contact parent to open an indirect connection (i.e. routed through multiple computers)
        transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "OPENROUTE", destination, modem.address, nil, modem.address)
        -- timeout of 8 seconds chosen to ensure that routing request will go through, even if all the way across network (tier 3->2->1->gateway->1->2->3)
        local eventName, receivingModem, sendingModem, port, distance, payload = event.pull(8, "modem_message")
        if payload == "ROUTE OPEN" then
            isOpen = true
        end
        if isOpen == true then
            connectNum = storeConnections(destination, modem.address, nil, neighbors[1]["address"], neighbors[1]["port"])
        end
        print(isOpen)
        return isOpen, connectNum
    end
end

local function getConnectionPair(origination, destination)
    local outgoingRoute = 0
    local incomingRoute = 0
    for key, value in pairs(connections) do
        if value["destination"] == destination and value["origination"] == origination then
            print("We found an outgoing connection!")
            outgoingRoute = key
            if outgoingRoute ~= 0 and incomingRoute ~= 0 then
                break
            end
        end
        
        if value["destination"] == origination and value["origination"] == destination then
            print("We found an incoming connection!")
            incomingRoute = key
            if incomingRoute ~= 0 and outgoingRoute ~= 0 then
                break
            end
        end
    end
    return outgoingRoute, incomingRoute
end

-- function to write data to an opened connection
local function writeData(destination, data, ...)
    if ... ~= nil then
        local connectNum = ...
        if connections[connectNum]["destination"] == destination then
            transmitInformation(connections[connectNum]["nextHop"], connections[connectNum]["port"], "DATA", data, destination, modem.address)
        end
    else
        local connectNum = 0
        for key, value in pairs(connections) do
            if value["destination"] == destination then
                connectNum = key
                break
            end
        end
        if connectNum ~= 0 then
            transmitInformation(connections[connectNum]["beforeHop"], connections[connectNum]["port"], "DATA", data, destination, modem.address)
        else
            print("This destination is not a recognized connection")
        end
    end
end

-- function to read data from an opened connection
local function readData(connectNum)
    local data = connections[connectNum]["data"]
    connections[connectNum]["data"] = {}
    connections[connectNum]["dataDex"] = 1
    return data
end

-- This is the function that allows end-users to open sockets. It will cache previously opened connections to allow for a faster re-opening. It also allows for the function to be called even when openRoute has not been called previously.
function GERTi.openSocket(destination)
    local origination = modem.address
    local incomingRoute = 0
    local outgoingRoute = 0
    local isValid = true
    local socket = {}
    outgoingRoute, incomingRoute = getConnectionPair(modem.address, destination)
    if incomingRoute == 0 or outgoingRoute == 0 then
        isValid = GERTi.openRoute(destination)
        if isValid == true then
            outgoingRoute, incomingRoute = getConnectionPair(modem.address, destination)
        end
    end
    if isValid = true then
        socket.origin = modem.address
        socket.destination = destination
        socket.incomingRoute = incomingRoute
        socket.outgoingRoute = outgoingRoute
        socket.write = writeData
        socket.read = readData
    else
        print("route cannot be opened, please confirm destination and that a valid path exists")
    end
    return socket, isValid
end
-- This is a simple function that returns the connection table for programs to examine
function GERTi.getConnections()
    return connections
end
function GERTi.getNeighbors()
    return neighbors
end
return GERTi
