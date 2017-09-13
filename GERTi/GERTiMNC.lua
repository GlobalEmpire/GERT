-- GERT v1.0 - build 3
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local GERTe = nil
local serialize = require("serialization")
local addressFile = "/usr/programs/addresses.cfg"
local modem = nil
local tunnel = nil

local childNodes = {}
local childNum = 1
local connections = {}
local connectDex = 1
local paths = {}
local pathDex = 1

local tier = 0
local handler = {}
local addressDex = 1
local gAddress = nil
local gKey = nil

local savedAddresses = {}
if (not component.isAvailable("tunnel")) and (not component.isAvailable("modem")) then
	io.stderr:write("This program requires a network or linked card to run.")
	os.exit(1)
end

if (component.isAvailable("modem")) then
	modem = component.modem
	modem.open(4378)

	if (component.modem.isWireless()) then
		modem.setStrength(500)
	end
end

if (component.isAvailable("tunnel")) then
	tunnel = component.tunnel
end

if filesystem.exists("/lib/GERTeAPI.lua") then
	GERTe = require("GERTeAPI")
end

-- functions to store the children and then sort the table
local function sortTable(elementOne, elementTwo)
	return (tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]))
end

-- this function adds a handler for a set time in seconds, or until that handler returns a truthful value (whichever comes first)
-- extremely useful for callback programming
local function addTempHandler(timeout, code, cb, cbf)
	local function cbi(...)
        local evn, rc, sd, pt, dt, code2 = ...
        if code ~= code2 then return end
        if cb(...) then
        	return false
        end
	end
        event.listen("modem_message", cbi)
        event.timer(timeout, function ()
                event.ignore("modem_message", cbi)
                cbf()
        end)
end

local function storeChild(rAddress, port, distance, package)
	-- parents means the direct connections a computer can make to another computer that is a higher tier than it
	-- children means the direct connections a comptuer can make to another computer that is a lower tier than it
	local childGAddress
	for key, value in pairs(savedAddresses) do
		if value["rAddress"] == rAddress then
			childGAddress = value["gAddress"]
			break
		end
	end
	childNodes[childNum] = {}
	childNodes[childNum]["realAddress"] = rAddress
	childNodes[childNum]["gAddress"] = (childGAddress or addressDex)
	childNodes[childNum]["tier"] = tonumber(package)
	childNodes[childNum]["port"] = tonumber(port)
	childNodes[childNum]["parents"] = {}
	childNodes[childNum]["children"]={}
	childNum = childNum + 1
	table.sort(childNodes, sortTable)
	if not childGAddress then
		savedAddresses[(#savedAddresses)+1] = {}
		savedAddresses[#savedAddresses]["rAddress"] = rAddress
		savedAddresses[#savedAddresses]["gAddress"] = addressDex
		local f = io.open(addressFile, "a")
		f:write(tostring(addressDex).."\n")
		f:write(childNodes[childNum-1]["realAddress"].."\n")
		f:close()
		addressDex = addressDex + 1
	end
	return (childNum-1), childNodes[childNum-1]["gAddress"]
end

local function removeChild(address)
	for key, value in pairs(childNodes) do
		if value["realAddress"] == address then
			table.remove(childNodes, key)
			childNum = (#childNodes+1)
			break
		end
	end
end

local function storeConnection(origination, destination, nextHop, outbound, connectionID)
	connections[connectDex] = {}
	connections[connectDex]["destination"] = destination
	connections[connectDex]["origination"] = origination
	connections[connectDex]["nextHop"] = nextHop
	connections[connectDex]["data"] = {}
	connections[connectDex]["dataDex"] = 1
	connections[connectDex]["outbound"] = outbound
	conncetions[connectDex]["connectionID"] = (connectionID or connectDex)
	connectDex = connectDex + 1
	return (connectDex-1)
end
local function storePath(origination, destination, nextHop, port)
	paths[pathDex] = {}
	paths[pathDex]["origination"] = origination
	paths[pathDex]["destination"] = destination
	paths[pathDex]["nextHop"] = nextHop
	paths[pathDex]["port"] = port
	pathDex = pathDex + 1
	return (pathDex-1)
end
local function storePaths(origination, destination, beforeHop, nextHop, beforePort, nextPort)
	return storePath(origination, destination, nextHop, nextPort), storePath(destination, origination, beforeHop, beforePort)
end

local function transmitInformation(sendTo, port, ...)
	if (port ~= 0) and (modem) then
		return modem.send(sendTo, port, ...)
	elseif (tunnel) then
		return tunnel.send(...)
	end
	io.stderr:write("Tried to transmit on tunnel, but no tunnel was found.")
	return false
end

handler["AddNeighbor"] = function (sendingModem, port, distance, code)
	local doesExist = false
	local childTier = 1
	print("GERTiStartReceived")
	for key,value in pairs(childNodes) do
		if value["realAddress"] == sendingModem then
			doesExist = true
			childNodes[key]["tier"] = childTier
			childNodes[key]["port"] = port
			childNodes[key]["children"] = {}
			childNodes[key]["parents"] = {}
			break
		end
	end
	if doesExist == false then
		storeChild(sendingModem, port, distance, childTier)
	end
	transmitInformation(sendingModem, port, "RETURNSTART", tier)
end

handler["DATA"] = function (sendingModem, port, distance, code, data, destination, origination, connectionID)
	for key, value in pairs(paths) do
		if value["destination"] == destination and value["origination"] == origination then
			if value["destination"] ~= modem.address then
				return transmitInformation(value["nextHop"], value["port"], "DATA", data, destination, origination, connectionID)
			else
				for key, value in pairs(connections) do
					if value["destination"] == destination and value["origination"] == origination then
						computer.pushSignal("DataOut", value["outbound"], data, connectionID)
						if GERTe then
							local gAddressOrigin = nil
							for key2, value2 in pairs(childNodes) do
								if value2["realAddress"] == value["origination"] then
									gAddressOrigin = value2["gAddress"]
									break
								end
							end
							GERTe.transmitTo(value["outbound"], gAddressOrigin, data)
						end
					end
				end
			end
		end
	end
	return false
end

-- Used in handler["OPENROUTE"]
local function routeOpener(destination, origination, beforeHop, hopOne, hopTwo, receivedPort, transmitPort, outbound, connectionID)
	print("Opening Route")
    local function sendOKResponse(isDestination)
		transmitInformation(beforeHop, receivedPort, "ROUTE OPEN", destination, origination)
		if isDestination then
			storeConnection(origination, destination, hopOne, outbound, connectionID)
			storePaths(origination, destination, beforeHop, hopOne, receivedPort, transmitPort)
		else
			storePaths(origination, destination, beforeHop, hopOne, receivedPort, transmitPort)
		end
	end
    if modem.address ~= destination then
		transmitInformation(hopOne, transmitPort, "OPENROUTE", destination, hopTwo, origination, outbound, connectionID)
        addTempHandler(3, "ROUTE OPEN", function (eventName, recv, sender, port, distance, code, pktDest, pktOrig)
        	if (destination == pktDest) and (origination == pktOrig) then
        		sendOKResponse(false)
            	return true -- This terminates the wait
			end
            end, function () end)
        return
    end
    sendOKResponse(true)
end

handler["OPENROUTE"] = function (sendingModem, port, distance, code, destination, intermediary, origination, outbound, connectionID)
	-- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
	if destination == modem.address then
		return routeOpener(destination, origination, sendingModem, modem.address, modem.address, port, port, outbound, connectionID)
	end

	local childKey = nil
	-- attempt to look up the node and establish a routing path
	for key, value in pairs(childNodes) do
		if value["realAddress"] == destination then
			childKey = key
			if childNodes[childKey]["parents"][1]["address"] == modem.address then
				return routeOpener(destination, origination, sendingModem, destination, destination, port, childNodes[childKey]["port"], outbound, connectionID)
			end
			break
		end
	end
	if (childKey) then
		-- now begin a search for an indirect connection, with support for up to 2 computers between the gateway and destination
		for key, value in pairs(childNodes[childKey]["parents"]) do
			for key2, value2 in pairs(childNodes) do
				if value2["realAddress"] == value["address"] and childNodes[key2]["parents"][1]["address"] == modem.address then
					-- If an intermediate is found, then use that to open a connection
					return routeOpener(destination, origination, sendingModem, value2["realAddress"], value2["realAddress"], port, value2["port"], outbound, connectionID)
				end
			end
		end

		-- If an intermediate is not found, attempt to do a 2-deep search for hops
		local parent1Key, parent2Key = nil
		local childParents = childNodes[childKey]["parents"]
		for key,value in pairs(childNodes) do
			for key2, value2 in pairs(value["children"]) do
				for key3, value3 in pairs(childParents) do
					-- so much nesting!
					if value3["address"] == value2["address"] then
						-- we now have the keys of the 2 computers, and the link will look like: gateway -- parent1Key -- parent2Key -- destination
						return routeOpener(destination, origination, sendingModem, value["realAddress"], value2["address"], port, childNodes[key]["port"], outbound, connectionID)
					end
				end
			end
		end 
	end
end

handler["RegisterNode"] = function (sendingModem, port, distance, code, originatorAddress, childTier, neighborTable)
	neighborTable = serialize.unserialize(neighborTable)
	local nodeDex = 0
	for key, value in pairs(childNodes) do
		if value["realAddress"] == originatorAddress then
			nodeDex = key
			break
		end
	end
	if nodeDex == 0 then
		nodeDex = storeChild(originatorAddress, port, distance, childTier)
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
	transmitInformation(sendingModem, port, "RegisterComplete", originatorAddress, childNodes[nodeDex]["gAddress"])
end

handler["RemoveNeighbor"] = function (sendingModem, port, distance, code, origination)
	removeChild(origination)
end

handler["ResolveAddress"] = function (sendingModem, port, distance, code, gAddress)
	if string.find(tostring(gAddress), ":") then
		return transmitInformation(sendingModem, port, "ResolveComplete", gAddress, (modem or tunnel).address)
	else
		for key, value in pairs(childNodes) do
			if tonumber(value["gAddress"]) == tonumber(gAddress) then
				return value["realAddress"], transmitInformation(sendingModem, port, "ResolveComplete", value["realAddress"])
			end
		end
	end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	if handler[code] ~= nil then
		handler[code](sendingModem, port, distance, code, ...)
	end
end
-- Functionality to allow reception of data from outside of the GERTi network, and then to pass it inwards
local function openExternalConnection(eventName, gDestination, origination)
	local rDestination = ""
	if string.find(tostring(gDestination), ":") then
		rDestination = handler["ResolveAddress"](nil, nil, modem.address, 4378, nil, nil, string.sub(gDestination, (string.find(tostring(gDestination), ":")+1)))
	else
		rDestination = handler["ResolveAddress"](nil, nil, modem.address, 4378, nil, nil, gDestination)
	end
	computer.pushSignal("modem_message", nil, modem.address, 4378, 1, "OPENROUTE", rDestination, nil, modem.address, origination)
	computer.pushSignal("addressReturn", rDestination)
end
event.listen("OpenRoute", openExternalConnection)

local function receiveData(eventName, data, destination)
	computer.pushSignal("modem_message", nil, nil, 4378, 1, "DATA", data, destination, modem.address)
end

local function readGMessage()
	-- keep calling GERTe.parse until it returns nil
	local message = GERTe.parse()
	while message ~= nil do
		local found = false
		local rDestination = handler["ResolveAddress"](nil, nil, modem.address, 4379, nil, nil, message["target"])
		for key, value in pairs(connections) do
			if value["destination"] == rDestination and value["origination"] == modem.address then
				handler["DATA"](nil, nil, nil, nil, nil, nil, message["data"], rDestination, modem.address)
				found = true
				break
			end
		end
		if not found then
			handler["OPENROUTE"](nil, nil, modem.address, 4378, nil, nil, rDestination, nil, modem.address, nil)
			handler["DATA"](nil, nil, nil, nil, nil, nil, message["data"], rDestination, modem.address)
		end
		message = GERTe.parse()
	end
end
------------------------ Startup procedure
event.listen("modem_message", receivePacket)
event.listen("DataIn", receiveData)

if GERTe then
	local file = io.open("/lib/GERTConfig.cfg", "r")
	gAddress = file:read()
	gKey = file:read()
	GERTe.startup()
	GERTe.register(gAddress, gKey)
	event.timer(0.1, readGMessage, math.huge)
end

if filesystem.exists(addressFile) then
	print("We found the file")
	local f = io.open(addressFile, "r")
	local counter = 1
	local newGAddress = f:read("*l")
	local newRAddress = f:read("*l")
	while newGAddress ~= nil do
		savedAddresses[counter] = {}
		savedAddresses[counter]["gAddress"] = newGAddress
		savedAddresses[counter]["rAddress"] = newRAddress
		counter = counter + 1
		newGAddress = f:read("*l")
		newRAddress = f:read("*l")
	end
	f:close()
	addressDex = counter
end