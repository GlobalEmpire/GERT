-- Under Construction
local GERTi = {}
local component = require("component")
local computer = require("computer")
local event = require("event")
local serialize = require("serialization")
local modem = nil
local tunnel = nil

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

-- addresses
local eAddress = 0
local iAddress = 0
local cachedAddress = {{}, {}, {}, {}, {}}
local addressDex = 1
-- Tables of neighbors and connections
local neighbors = {}
local tier = 3
local neighborDex = 1
local connections = {}
local connectDex = 1
local handler = {}

-- internal functions
local function sortTable(elementOne, elementTwo)
	return (tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]))
end

local function storeNeighbors(eventName, receivingModem, sendingModem, port, distance, package)
	-- Register neighbors for communication to the rest of the network
	neighbors[neighborDex] = {}
	neighbors[neighborDex]["address"] = sendingModem
	neighbors[neighborDex]["port"] = tonumber(port)
	if package == nil then
		--This is used for when a computer receives a new client's AddNeighbor message. It stores a neighbor connection with a tier one lower than this computer's tier
		neighbors[neighborDex]["tier"] = (tier+1)
	else
		-- This is used for when a computer receives replies to its AddNeighbor message.
		neighbors[neighborDex]["tier"] = tonumber(package)
		if tonumber(package) < tier then
			-- attempt to set this computer's tier to one lower than the highest ranked neighbor.
			tier = tonumber(package)+1
		end
	end
	neighborDex = neighborDex + 1
	-- sort table so that the best connection to the Master Network Controller (MNC) comes first
	table.sort(neighbors, sortTable)

	return true
end

local function removeNeighbor(address)
	for key, value in pairs(neighbors) do
		if value["address"] == address then
			table.remove(neighbors, key)
			break
		end
	end
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

-- Calls storeConnection twice to create a pair of uni-directional connections
local function storeConnections(origination, destination, beforeHop, nextHop, beforePort, nextPort)
	return (storeConnection(origination, destination, beforeHop, nextHop, beforePort) and storeConnection(destination, origination, nextHop, beforeHop, nextPort))
end

-- Stores data inside a connection for use by a program
local function storeData(connectNum, data)
	local dataNum = connections[connectNum]["dataDex"]

	if dataNum >= 20 then
		connections[connectNum]["dataDex"], dataNum = 1
	end

	connections[connectNum]["data"][dataNum]=data
	connections[connectNum]["dataDex"] = dataNum + 1

	return true
end

local function cacheAddress(gAddress, realAddress)
	if addressDex > 5 then
		addressDex = 1
		return cacheAddress(gAddress, realAddress)
	end
	cachedAddress[addressDex]["gAddress"] = gAddress
	cachedAddress[addressDex]["realAddress"] = realAddress
end
-- Low level function that abstracts away the differences between a wired/wireless network card and linked card.
local function transmitInformation(sendTo, port, ...)
	if (port ~= 0) and (modem) then
		return modem.send(sendTo, port, ...)
	elseif (tunnel) then
		return tunnel.send(...)
	end

	io.stderr:write("Tried to transmit, but no network card or linked card was found.")
	return false
end

handler["AddNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code)
	-- Process AddNeighbor messages and add them as neighbors
	if tier < 3 then
		storeNeighbors(eventName, receivingModem, sendingModem, port, distance, nil)
		return transmitInformation(sendingModem, port, "RETURNSTART", tier)
	end
	return false
end

-- Handlers that manage incoming packets after processing
handler["DATA"] = function (eventName, receivingModem, sendingModem, port, distance, code, data, destination, origination)
	-- Attempt to determine if host is the destination, else send it on to next hop.
	for key, value in pairs(connections) do
		if value["destination"] == destination and value["origination"] == origination then
			if connections[key]["destination"] == modem.address then
				return storeData(key, data)
			else
				return transmitInformation(connections[key]["nextHop"], connections[key]["port"], "DATA", data, destination, origination)
			end
		end
	end
	return false
end

-- opens a route using the given information, used in handler["OPENROUTE"] and GERTi.openRoute()
-- DIFFERs from MNC's orController
local function orController(destination, origination, beforeHop, nextHop, receivedPort, transmitPort)
	print("Opening Route")
	if modem.address ~= destination then
		print("modem address was not destination")
		transmitInformation(nextHop, transmitPort, "OPENROUTE", destination, nextHop, origination)
		local eventName, receivingModem, _, port, distance, payload = event.pull(6, "modem_message")
		if payload ~= "ROUTE OPEN" then
			return false
		end
	end
	return transmitInformation(beforeHop, receivedPort, "ROUTE OPEN"), storeConnections(origination, destination, beforeHop, nextHop, receivedPort, transmitPort)
end

handler["OPENROUTE"] = function (eventName, receivingModem, sendingModem, port, distance, code, destination, intermediary, origination)
	-- Attempt to determine if the intended destination is this computer
	if destination == modem.address then
		return orController(modem.address, origination, sendingModem, modem.address, port, port)
	end

	-- attempt to check if destination is a neighbor to this computer, if so, re-transmit OPENROUTE message to the neighbor so routing can be completed
	for key, value in pairs(neighbors) do
		if value["address"] == destination then
			return orController(destination, origination, sendingModem, neighbors[key]["address"], port, neighbors[key]["port"])
		end
	end

	-- if it is not a neighbor, and no intermediary was found, then contact parent to forward indirect connection request
	if intermediary == modem.address then
		return orController(destination, origination, sendingModem, neighbors[1]["address"], port, neighbors[1]["port"])
	end

	-- If an intermediary is found (likely because MNC was already contacted), then attempt to forward request to intermediary
	for key, value in pairs(neighbors) do
		if value["address"] == intermediary then
			return orController(destination, origination, sendingModem, intermediary, port, neighbors[key]["port"])
		end
	end

	return false
end

handler["RemoveNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code, origination)
	removeNeighbor(origination)
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RemoveNeighbor", origination)
end

handler["RegisterNode"] = function (eventName, receivingModem, sendingModem, port, distance, code, origination, tier, serialTable)
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RetrieveAddress", origination, tier, serialTable)
	local _, _, _, port, _, payload = event.pull(5, "modem_message")
	if payload ~= nil then
		return transmitInformation(sendingModem, port, payload)
	end
end

handler["RETURNSTART"] = function (eventName, receivingModem, sendingModem, port, distance, code, tier)
	-- Store neighbor based on the returning tier
	return storeNeighbors(eventName, receivingModem, sendingModem, port, distance, tier)
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	-- Attempt to call a handler function to further process the packet
	if handler[code] ~= nil then
		return handler[code](eventName, receivingModem, sendingModem, port, distance, code, ...)
	end
end

-- Begin startup ---------------------------------------------------------------------------------------------------------------------------
-- transmit broadcast to check for neighboring GERTi enabled computers
if tunnel then
	tunnel.send("AddNeighbor")
	receivePacket(event.pull(1, "modem_message"))
end

if modem then
	modem.broadcast(4378, "AddNeighbor")
	local continue = true
	while continue do
		continue = receivePacket(event.pull(1, "modem_message"))
	end
end

-- forward neighbor table up the line
local serialTable = serialize.serialize(neighbors)
if serialTable ~= "{}" then
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RegisterNode", modem.address, tier, serialTable)
	local _, _, _, _, _, payload = event.pull(5, "modem_message")
	if payload ~= nil then
		iAddress = payload
	else
		print("Master Network Controller could not be found. This may result in impaired network functionality.")
	end
end

-- Register event listener to receive packets from now on
event.listen("modem_message", receivePacket)

--Override computer.shutdown to allow for better network leaves
local function safedown()
	if tunnel then
		tunnel.send("RemoveNeighbor", modem.address)
	end
	if modem then
		modem.broadcast(4378, "RemoveNeighbor", modem.address)
	end
end
event.listen("shutdown", safedown)
-- startup procedure is now complete ------------------------------------------------------------------------------------------------------------

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
		return orController(neighbors[neighborKey]["address"], modem.address, modem.address, neighbors[neighborKey]["address"], neighbors[neighborKey]["port"], neighbors[neighborKey]["port"])
	else
		-- if neighbor is not local, then attempt to contact parent to open an indirect connection (i.e. routed through multiple computers)
		return orController(destination, modem.address, modem.address, neighbors[1]["address"], neighbors[1]["port"], neighbors[1]["port"])
	end
end

-- Attempts to return a pair of connections between an origin and destination for use in openSocket
local function getConnectionPair(origination, destination)
	local outgoingRoute = nil
	local incomingRoute = nil
	local outgoingPort = nil
	local incomingPort = nil
	
	for key, value in pairs(connections) do
		if not outgoingRoute and value["destination"] == destination and value["origination"] == origination then
			print("We found an outgoing connection!")
			outgoingRoute = key
			outgoingPort = value["port"]
		end

		if not incomingRoute and value["destination"] == origination and value["origination"] == destination then
			print("We found an incoming connection!")
			incomingRoute = key
			incomingPort = value["port"]
		end
	end

	if incomingRoute and outgoingRoute then
		return outgoingRoute, incomingRoute, outgoingPort, incomingPort
	end

	return 0,0
end

-- Writes data to an opened connection
local function writeData(self, data)
	return transmitInformation(connections[self.outgoingRoute]["nextHop"], self.outPort, "DATA", data, self.destination, modem.address)
end

-- Reads data from an opened connection
local function readData(self)
	local data = connections[self.incomingRoute]["data"]
	connections[self.incomingRoute]["data"] = {}
	connections[self.incomingRoute]["dataDex"] = 1
	return data
end

-- This is the function that allows end-users to open sockets. It will cache previously opened connections to allow for a faster re-opening. It also allows for the function to be called even when openRoute has not been called previously.
function GERTi.openSocket(gAddress)
	local realAddress = nil
	local origination = modem.address
	local incomingRoute = 0
	local outgoingRoute = 0
	local outgoingPort = 0
	local incomingPort = 0
	local isValid = true
	local socket = {}
	-- attempt to check cached addresses, otherwise contact MNC for address resolution
	for key, value in pairs(cachedAddress) do
		if value["gAddress"] == gAddress then
			realAddress = value["realAddress"]
			break
		end
	end
	if realAddress == nil then
		transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "ResolveAddress", gAddress)
		local _, _, _, _, _, payload = event.pull(5, "modem_message")
		if payload ~= nil then
			cacheAddress(gAddress, payload)
			realAddress = payload
		else
			print("This is not a valid address")
			return "invalid address"
		end
	end
	outgoingRoute, incomingRoute, outgoingPort, incomingPort = getConnectionPair(modem.address, realAddress)
	if incomingRoute == 0 or outgoingRoute == 0 then
		isValid = GERTi.openRoute(realAddress)
		if isValid == true then
			outgoingRoute, incomingRoute, outgoingPort, incomingPort = getConnectionPair(modem.address, realAddress)
		end
	end
	if isValid == true then
		socket.origin = modem.address
		socket.destination = realAddress
		socket.incomingRoute = incomingRoute
		socket.outgoingRoute = outgoingRoute
		socket.outPort = outgoingPort
		socket.inPort = incomingPort
		socket.write = writeData
		socket.read = readData
	else
		print("Route cannot be opened, please confirm destination and that a valid path exists.")
	end
	return socket, isValid
end

function GERTi.getConnections()
	return connections
end

function GERTi.getNeighbors()
	return neighbors
end

function GERTi.getAddress()
	return iAddress
end
return GERTi
