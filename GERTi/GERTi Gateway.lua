-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")

local childNodes = {}
local childNum = 1
local connections = {}
local connectDex = 1
local tier = 0
local handler = {}

if (not component.isAvailable("tunnel")) and (not component.isAvailable("modem")) then
    io.stderr:write("This program requires a network card to run.")
end

if (component.isAvailable("modem")) then
    modem = component.modem
    modem.open(4378)

    if (component.modem.isWireless()) then
        modem.setStrength(500)
    end
else
    modem = false
end

if (component.isAvailable("tunnel")) then
    tunnel = component.tunnel
else
    tunnel = false
end

-- functions to store the children and then sort the table
local function sortTable(elementOne, elementTwo)
    if tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]) then
        return true
    else
        return false
    end
end

local function storeChild(eventName, receivingModem, sendingModem, port, distance, package)
    -- register neighbors for communication to gateway
    -- parents means the direct connections a computer can make to another computer that is a higher tier than it
    -- children means the direct connections a comptuer can make to another computer that is a lower tier than it
    childNodes[childNum] = {}
    childNodes[childNum]["address"] = sendingModem
    childNodes[childNum]["tier"] = tonumber(package)
    childNodes[childNum]["port"] = tonumber(port)
    childNodes[childNum]["parents"] = {}
    childNodes[childNum]["children"]={}
    print("inside store Child")
    print(childNodes[childNum]["address"])
    childNum = childNum + 1
    table.sort(childNodes, sortTable)
    return (childNum-1)
end

local function storeConnection(destination, origination, beforeHop, nextHop, port)
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

local function transmitInformation(sendTo, port, ...)
    if (port ~= 0) and (modem) then
        modem.send(sendTo, port, ...)
    elseif (tunnel) then
        tunnel.send(...)
    else
        io.stderr:write("Tried to transmit on tunnel, but no tunnel was found.")
    end
end

handler["DATA"] = function (eventName, receivingModem, sendingModem, port, distance, code, data, destination, origination)
    local connectNum = 0
        for key, value in pairs(connections) do
            if value["destination"] == destination and value["origination"] == origination then
                connectNum = key
                break
            end
        end
        if connectNum ~= 0 then
            -- connectNum should never ever be 0, but I don't even know these days.
            if connections[connectNum]["destination"] ~= modem.address then
                transmitInformation(connections[connectNum]["nextHop"], connections[connectNum]["port"], "DATA", data, destination, origination)
            end
        end
		return true
end

handler["OPENROUTE"] = function (eventName, receivingModem, sendingModem, port, distance, code, destination, intermediary, intermediary2, origination)
    local childKey = 0
        -- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
    if destination == modem.address then
        storeConnections(origination, modem.address, sendingModem, modem.address, port)
        transmitInformation(sendingModem, port, "ROUTE OPEN")
        print("opening route")
    else
        -- attempt to look up the node and establish a routing path
        for key, value in pairs(childNodes) do
            if value["address"] == destination then
                childKey = key
                break
            end
        end
        -- attempt to determine if the gateway is a direct parent of the intended destination
        if childKey ~= 0 then
            local gateParent = false
            for key, value in pairs(childNodes[childKey]["parents"]) do
                if value["address"] == modem.address then
                    gateParent = true
                    break
                end
            end
            -- if gateway is direct parent, open direct connection, otherwise open an indirect connection
            if gateParent == true then
                transmitInformation(childNodes[childKey]["address"], childNodes[childKey]["port"], "OPENROUTE", destination, modem.address, nil, origination)
                local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                if payload == "ROUTE OPEN" then
                    storeConnections(origination, destination, sendingModem, destination, port)
                    transmitInformation(sendingModem, port, "ROUTE OPEN")
                end
            else
                -- now begin a search for an indirect connection, with support for up to 2 computers between the gateway and destination
                local parent1Key, parent2Key = 0
                for key, value in pairs(childNodes[childKey]["parents"]) do
                    for key2, value2 in pairs(childNodes) do
                        if value2["address"] == value["address"] and childNodes[key2]["parents"][1]["address"] == modem.address then
                            parent1Key = key2
                            break
                        end
                    end
                    if parent1Key ~= 0 then
                        break
                    end
                end
                if parent1Key ~= 0 then
                    -- If an intermediate is found, then use that to open a direct connection
                    transmitInformation(childNodes[parent1Key]["address"], childNodes[parent1Key]["port"], "OPENROUTE", destination, modem.address, destination, origination)
                    local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
                    if payload == "ROUTE OPEN" then
                        storeConnections(origination, destination, sendingModem, childNodes[parent1Key]["address"], port)
                        transmitInformation(sendingModem, port, "ROUTE OPEN")
                    end
                else
                    -- If an intermediate is not found, attempt to do a 2-deep search for hops
                    local childParents = childNodes[childKey]["parents"]
                    for key,value in pairs(childNodes) do
                        for key2, value2 in pairs(value["children"]) do
                            for key3, value3 in pairs(childParents) do
                                -- so much nesting!
                                if value3["address"] == value2["address"] then
                                    parent2Key = key3
                                    parent1Key = key
                                    break
                                end
                            end
                            if parent2Key ~= 0 then
                                break
                            end
                        end
                        if parent2Key ~= 0 then
                            break
                        end
                    end
                    -- we now have the keys of the 2 computers, and the link will look like: gateway -- parent1Key -- parent2Key -- destination
                    transmitInformation(childNodes[parent1Key]["address"], childNodes[parent1Key]["port"], "OPENROUTE", destination, modem.address, childNodes[parent2Key]["address"], origination)
                    local eventName, receivingModem, _, port, distance, payload = event.pull(7, "modem_message")
                    if payload == "ROUTE OPEN" then
                        storeConnections(origination, destination, sendingModem, childNodes[parent1Key]["address"], port)
                        transmitInformation(sendingModem, port, "ROUTE OPEN")
                    end
                end
            end
        end
    end
end

handler["GERTiStart"] = function (eventName, receivingModem, sendingModem, port, distance, code)
    local doesExist = false
    local childTier = 1
    print("GERTiStartReceived")
    for key,value in pairs(childNodes) do
        if value["address"] == sendingModem then
            doesExist = true
            childNodes[key]["tier"] = childTier
            childNodes[key]["port"] = port
            childNodes[key]["children"] = {}
            childNodes[key]["parents"] = {}
            break
        end
    end
    if doesExist == false then
        storeChild(eventName, receivingModem, sendingModem, port, distance, childTier)
    end
    transmitInformation(sendingModem, port, "RETURNSTART", tier)
end

handler["GERTiForwardTable"] = function (eventName, receivingModem, sendingModem, port, distance, code, originatorAddress, childTier, neighborTable)
    neighborTable = serialize.unserialize(neighborTable)
    local nodeDex = 0
    for key, value in pairs(childNodes) do
        if value["address"] == originatorAddress then
            nodeDex = key
            break
        end
    end
    if nodeDex == 0 then
        nodeDex = storeChild(eventName, receivingModem, sendingModem, port, distance, childTier)
    end
    local parentDex = 1
    local subChildDex = 1
    for key, value in pairs(neighborTable) do
        if neighborTable[key]["tier"] < childTier then
            childNodes[nodeDex]["parents"][parentDex]=value
            parentDex = parentDex + 1
        else
            childNodes[nodeDex]["children"][subChildDex]=value
            subChildDex = subChildDex + 1
        end
    end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
    print(code)
    if handler[code] ~= nil then
        return handler[code](eventName, receivingModem, sendingModem, port, distance, code, ...)
    else
        return false
    end
end

event.listen("modem_message", receivePacket)
