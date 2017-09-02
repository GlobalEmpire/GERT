-- Under Construction
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local GERTe = nil
local serialize = require("serialization")
local modem = nil
local tunnel = nil

local childNodes = {}
local childNum = 1
local connections = {}
local connectDex = 1
local tier = 0
local handler = {}
local addressDex = 1
local gAddress = nil
local gKey = nil
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
                event.ignore(cbi)
                cbf()
        end)
end

local function storeChild(eventName, receivingModem, sendingModem, port, distance, package)
	-- register neighbors for communication to gateway
	-- parents means the direct connections a computer can make to another computer that is a higher tier than it
	-- children means the direct connections a comptuer can make to another computer that is a lower tier than it
	childNodes[childNum] = {}
	childNodes[childNum]["realAddress"] = sendingModem
	childNodes[childNum]["gAddress"] = addressDex
	childNodes[childNum]["tier"] = tonumber(package)
	childNodes[childNum]["port"] = tonumber(port)
	childNodes[childNum]["parents"] = {}
	childNodes[childNum]["children"]={}
	print("inside store Child")
	print(childNodes[childNum]["realAddress"])
	childNum = childNum + 1
	addressDex = addressDex + 1
	table.sort(childNodes, sortTable)
	return (childNum-1), childNodes[childNum-1]["gAddress"]
end

local function removeChild(address)
	for key, value in pairs(childNodes) do
		if value["realAddress"] == address then
			table.remove(childNodes, key)
			break
		end
	end
end

local function storeConnection(destination, origination, beforeHop, nextHop, port, ...)
	connections[connectDex] = {}
	connections[connectDex]["destination"] = destination
	connections[connectDex]["origination"] = origination
	connections[connectDex]["beforeHop"] = beforeHop
	connections[connectDex]["nextHop"] = nextHop
	connections[connectDex]["port"] = port
	connections[connectDex]["data"] = {}
	connections[connectDex]["dataDex"] = 1
	connections[connectDex]["outbound"] = ...
	connectDex = connectDex + 1
	return (connectDex-1)
end

local function storeConnections(origination, destination, beforeHop, nextHop, port, ...)
	storeConnection(origination, destination, nextHop, beforeHop, port, ...)
	storeConnection(destination, origination, beforeHop, nextHop, port, ...)
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

handler["AddNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code)
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
		storeChild(eventName, receivingModem, sendingModem, port, distance, childTier)
	end
	transmitInformation(sendingModem, port, "RETURNSTART", tier)
end

handler["DATA"] = function (eventName, receivingModem, sendingModem, port, distance, code, data, destination, origination)
	for key, value in pairs(connections) do
		if value["destination"] == destination and value["origination"] == origination then
			if connections[key]["destination"] ~= modem.address then
				return transmitInformation(connections[key]["nextHop"], connections[key]["port"], "DATA", data, destination, origination)
			else
				computer.pushSignal("DataOut", value["outbound"], data)
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
	return false
end

-- Used in handler["OPENROUTE"]
-- MNC's openRoute and Client's openRoute DIFFER
local function orController(destination, origination, beforeHop, hopOne, hopTwo, receivedPort, transmitPort, cb, outbound)
        print("Opening Route")
        local function sendOKResponse()
                transmitInformation(beforeHop, receivedPort, "ROUTE OPEN", destination, origination)
		storeConnections(origination, destination, beforeHop, nextHop, receivedPort, transmitPort)
                if cb then
                        cb(true)
                end
        end
        if modem.address ~= destination then
		transmitInformation(hopOne, transmitPort, "OPENROUTE", destination, hopTwo, origination)
                -- Basically: When the ROUTE OPEN comes along, check if it's the route we want (it's one route per pair anyway)
                addTempHandler(6, "ROUTE OPEN", function (eventName, recv, sender, port, distance, code, pktDest, pktOrig)
                        if (destination == pktDest) and (origination == pktOrig) then
                                sendOKResponse()
                                return true -- This terminates the wait
                        end
                end, function () if cb then cb(false) end end)
                return
        end
        sendOKResponse()
end


handler["OPENROUTE"] = function (eventName, receivingModem, sendingModem, port, distance, code, destination, intermediary, origination, outbound)
	-- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
	if destination == modem.address then
		orController(destination, origination, sendingModem, modem.address, modem.address, port, port, nil, outbound)
		return
	end

	local childKey = nil
	-- attempt to look up the node and establish a routing path
	for key, value in pairs(childNodes) do
		if value["realAddress"] == destination then
			childKey = key
			if childNodes[childKey]["parents"][1]["address"] == modem.address then
				return orController(destination, origination, sendingModem, destination, destination, port, childNodes[childKey]["port"], nil)
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
					return orController(destination, origination, sendingModem, value2["realAddress"], value2["realAddress"], port, value2["port"], nil)
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
						return orController(destination, origination, sendingModem, value["realAddress"], value2["address"], port, childNodes[key]["port"], nil)
					end
				end
			end
		end 
	end
end

handler["RegisterNode"] = function (eventName, receivingModem, sendingModem, port, distance, code, originatorAddress, childTier, neighborTable)
	neighborTable = serialize.unserialize(neighborTable)
	local nodeDex = 0
	for key, value in pairs(childNodes) do
		if value["realAddress"] == originatorAddress then
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
	transmitInformation(sendingModem, port, "RegisterComplete", originatorAddress, childNodes[nodeDex]["gAddress"])
end

handler["RemoveNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code, origination)
	removeChild(origination)
end

handler["ResolveAddress"] = function (eventName, receivingModem, sendingModem, port, distance, code, gAddress)
	if string.find(tostring(gAddress), ":") then
		return transmitInformation(sendingModem, port, "ResolveComplete", gAddress, (modem or tunnel).address)
	else
		for key, value in pairs(childNodes) do
			if tonumber(value["gAddress"]) == tonumber(gAddress) then
				return value["realAddress"], transmitInformation(sendingModem, port, "ResolveComplete", gAddress)
			end
		end
	end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	if handler[code] ~= nil then
		handler[code](eventName, receivingModem, sendingModem, port, distance, code, ...)
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
