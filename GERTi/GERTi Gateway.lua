-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")
local modem = nil
local tunnel = nil

local childNodes = {}
local childNum = 1
local connections = {}
local connectDex = 1
local tier = 0
local handler = {}

if (not component.isAvailable("tunnel")) and (not component.isAvailable("modem")) then
	io.stderr:write("This program requires a network card to run.")
	return 1
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

-- functions to store the children and then sort the table
local function sortTable(elementOne, elementTwo)
	return (tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]))
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
		return modem.send(sendTo, port, ...)
	elseif (tunnel) then
		return tunnel.send(...)
	end
	io.stderr:write("Tried to transmit on tunnel, but no tunnel was found.")
	return false
end

handler["DATA"] = function (eventName, receivingModem, sendingModem, port, distance, code, data, destination, origination)
		for key, value in pairs(connections) do
			if value["destination"] == destination and value["origination"] == origination then
				if connections[key]["destination"] ~= modem.address then
					return transmitInformation(connections[key]["nextHop"], connections[key]["port"], "DATA", data, destination, origination)
				end
			end
		end
		return false
end

-- Used in handler["OPENROUTE"]
-- Gateway's openRoute and Client's openRoute DIFFER
local function openRoute(origination, destination, sendingModem, destination2, port, transmitAddress, transmitPort, transmitDestination)
	print("Opening Route")
	if modem.address ~= destination then
		transmitInformation(transmitAddress, transmitPort, "OPENROUTE", destination, modem.address, transmitDestination, origination)
		local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
		if payload ~= "ROUTE OPEN" then
			return false
		end
	end
	storeConnections(origination, destination, sendingModem, destination2, port)
	return transmitInformation(sendingModem, port, "ROUTE OPEN")
end

handler["OPENROUTE"] = function (eventName, receivingModem, sendingModem, port, distance, code, destination, intermediary, intermediary2, origination)
	-- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
	if destination == modem.address then
		return openRoute(origination, modem.address, sendingModem, modem.address, port, nil, nil, nil)
	end

	local childKey = nil
	-- attempt to look up the node and establish a routing path
	for key, value in pairs(childNodes) do
		if value["address"] == destination then
			childKey = key
			-- attempt to determine if the gateway is a direct parent of the intended destination
			for key2, value2 in pairs(childNodes[childKey]["parents"]) do
				if value2["address"] == modem.address then
					return openRoute(origination, destination, sendingModem, destination, port, childNodes[childKey]["address"], childNodes[childKey]["port"], nil)
				end
			end
		end
	end
		-- if gateway is direct parent, open direct connection, otherwise open an indirect connection
			-- now begin a search for an indirect connection, with support for up to 2 computers between the gateway and destination
	for key, value in pairs(childNodes[childKey]["parents"]) do
		for key2, value2 in pairs(childNodes) do
			if value2["address"] == value["address"] and childNodes[key2]["parents"][1]["address"] == modem.address then
				-- If an intermediate is found, then use that to open a direct connection
				return openRoute(origination, destination, sendingModem, childNodes[key2]["address"], port, childNodes[key2]["address"], childNodes[key2]["port"], destination)
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
					return openRoute(origination, destination, sendingModem, childNodes[key]["address"], port, childNodes[key3]["address"], childNodes[key]["port"], childNodes[key3]["address"])
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
