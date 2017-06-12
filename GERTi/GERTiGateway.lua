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

local function removeChild(address)
	for key, value in pairs(childNodes) do
		if value["address"] == address then
			table.remove(childNodes, key)
			break
		end
	end
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
	storeConnection(origination, destination, nextHop, beforeHop, port)
	storeConnection(destination, origination, beforeHop, nextHop, port)
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
local function orController(destination, origination, beforeHop, hopOne, hopTwo, receivedPort, transmitPort)
	print("Opening Route")
	if modem.address ~= destination then
		transmitInformation(hopOne, transmitPort, "OPENROUTE", destination, hopTwo, origination)
		local eventName, receivingModem, _, port, distance, payload = event.pull(2, "modem_message")
		if payload ~= "ROUTE OPEN" then
			return false
		end
	end
	storeConnections(origination, destination, beforeHop, hopOne, receivedPort)
	return transmitInformation(beforeHop, receivedPort, "ROUTE OPEN")
end

handler["OPENROUTE"] = function (eventName, receivingModem, sendingModem, port, distance, code, destination, intermediary, origination)
	-- attempt to check if destination is this computer, if so, respond with ROUTE OPEN message so routing can be completed
	if destination == modem.address then
		return orController(destination, origination, sendingModem, modem.address, port, nil)
	end

	local childKey = nil
	-- attempt to look up the node and establish a routing path
	for key, value in pairs(childNodes) do
		if value["address"] == destination then
			childKey = key
			if childNodes[childKey]["parents"][1]["address"] == modem.address then
				return orController(destination, origination, sendingModem, destination, destination, port, childNodes[childKey]["port"])
			end
			break
		end
	end
	if (childKey) then
		-- now begin a search for an indirect connection, with support for up to 2 computers between the gateway and destination
		for key, value in pairs(childNodes[childKey]["parents"]) do
			for key2, value2 in pairs(childNodes) do
				if value2["address"] == value["address"] and childNodes[key2]["parents"][1]["address"] == modem.address then
					-- If an intermediate is found, then use that to open a connection
					return orController(destination, origination, sendingModem, value2["address"], value2["address"], port, value2["port"])
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
						return orController(destination, origination, sendingModem, value["address"], value2["address"], port, childNodes[key]["port"])
					end
				end
			end
		end 
	else
		return false
	end
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

handler["RemoveNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code, origination)
	removeChild(origination)
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	if handler[code] ~= nil then
		handler[code](eventName, receivingModem, sendingModem, port, distance, code, ...)
	end
end

event.listen("modem_message", receivePacket)
