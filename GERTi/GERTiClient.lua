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

-- This only matters to the "GERTi" API way down below. Should be int - see startup procedure.
-- It is updated by handler["RegisterComplete"] if the message concerns this node.
local iAddress = nil
local cachedAddress = {{}, {}, {}, {}, {}}
local addressDex = 1
-- Tables of neighbors and connections
-- neighbors[x]{
--  address
--  port
--  tier
-- }
local neighbors = {}
local tier = 3
local neighborDex = 1
-- connections[x]{
--  destination
--  origin
--  beforeHop
--  nextHop
--  port
--  data
--  dataDex
-- }
local connections = {}
local connectDex = 1

-- Addresses awaiting resolution. Key is the gAddress, value is the function.
-- This is as opposed to addTempHandler which has no caching.
-- Add to this with addWaitingAddress.
-- These are cancelled after a set time.
local waitingAddress = {}

local handler = {}

-- internal functions
local function sortTable(elementOne, elementTwo)
	return (tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]))
end

-- like sortTable : these are common between MNC & GERTiClient
-- this function adds a handler for a set time in seconds, or until that handler returns a truthful value (whichever comes first)
-- extremely useful for callback programming
local function addTempHandler(timeout, code, cb, cbf)
	local disable = false
	local function cbi(...)
		if disable then return end
		local evn, rc, sd, pt, dt, code2 = ...
		if code ~= code2 then return end
		if cb(...) then
			-- Returning false from the event handler (specifically false) will get rid of the handler.
			-- I have no idea why an event handler in this code was set up to cause it to return false,
			--  apart from this one (because it wasn't *the handler holding everything together*)
			disable = true
			return false
		end
	end
	event.listen("modem_message", cbi)
	event.timer(timeout, function ()
		event.ignore(cbi)
		if disable then return end
		cbf()
	end)
end
-- Like a sleep, but it will exit early if a modem_message is received and then something happens.
local function waitWithCancel(timeout, cancelCheck)
	-- Wait for the response.
	local now = computer.uptime()
	local deadline = now + 5
	while now < deadline do
		event.pull(deadline - now, "modem_message")
		-- The listeners were called, so as far as we're concerned anything cancel-worthy should have happened
		local response = cancelCheck()
		if response then return response end
		now = computer.uptime()
	end
	-- Out of time
	return cancelCheck()
end
--

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

-- Sends (or reuses) an existing address resolution request.
-- Only use when MNC is present.
local function addWaitingAddress(gAddress, callback)
	if waitingAddress[gAddress] then
		local old = waitingAddress[gAddress]
		waitingAddress[gAddress] = function (response) callback(response) old(response) end
	else
		waitingAddress[gAddress] = callback
		event.timer(5, function ()
			-- Purge this receiver.
			waitingAddress[gAddress] = nil
		end)
		transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "ResolveAddress", gAddress)
	end
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
			if connections[key]["destination"] == (modem or tunnel).address then
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
-- 'cb' (callback) is given success (true) or failure (false)
-- The ... is sent in the MNC's direction
local function orController(destination, origination, beforeHop, nextHop, receivedPort, transmitPort, cb, ...)
	print("Opening Route")
	local function sendOKResponse()
		transmitInformation(beforeHop, receivedPort, "ROUTE OPEN", destination, origination)
		storeConnections(origination, destination, beforeHop, nextHop, receivedPort, transmitPort)
		if cb then
			cb(true)
		end
	end
	if modem.address ~= destination then
		print("modem address was not destination")
		transmitInformation(nextHop, transmitPort, "OPENROUTE", destination, nextHop, origination, ...)
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
	-- Attempt to determine if the intended destination is this computer
	if destination == modem.address then
		orController(modem.address, origination, sendingModem, modem.address, port, port, nil)
		return
	end

	-- attempt to check if destination is a neighbor to this computer, if so, re-transmit OPENROUTE message to the neighbor so routing can be completed
	for key, value in pairs(neighbors) do
		if value["address"] == destination then
			orController(destination, origination, sendingModem, neighbors[key]["address"], port, neighbors[key]["port"], nil)
			return
		end
	end

	-- if it is not a neighbor, and no intermediary was found, then contact parent to forward indirect connection request
	if intermediary == modem.address then
		orController(destination, origination, sendingModem, neighbors[1]["address"], port, neighbors[1]["port"], nil, outbound)
		return
	end

	-- If an intermediary is found (likely because MNC was already contacted), then attempt to forward request to intermediary
	for key, value in pairs(neighbors) do
		if value["address"] == intermediary then
			orController(destination, origination, sendingModem, intermediary, port, neighbors[key]["port"], nil)
			return
		end
	end
end

handler["RemoveNeighbor"] = function (eventName, receivingModem, sendingModem, port, distance, code, origination)
	removeNeighbor(origination)
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RemoveNeighbor", origination)
end

handler["RegisterNode"] = function (eventName, receivingModem, sendingModem, sendingPort, distance, code, origination, tier, serialTable)
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RegisterNode", origination, tier, serialTable)
	addTempHandler(5, "RegisterComplete", function (eventName, recv, sender, port, distance, code, targetMA, iResponse)
		if targetMA == origination then
			transmitInformation(sendingModem, sendingPort, "RegisterComplete", targetMA, iResponse)
			return true
		end
	end, function () end)
end

handler["RETURNSTART"] = function (eventName, receivingModem, sendingModem, port, distance, code, tier)
	-- Store neighbor based on the returning tier
	storeNeighbors(eventName, receivingModem, sendingModem, port, distance, tier)
end

handler["ResolveAddress"] = function (eventName, receivingModem, sendingModem, port, distance, code, gAddress)
	addWaitingAddress(gAddress, function(response)
		-- this indentation is weird.
		-- Anyway, this is the callback to forward the resolution
		transmitInformation(sendingModem, port, "AddressResolution", gAddress, response)
	end)
end

handler["AddressResolution"] = function (eventName, receivingModem, sendingModem, port, distance, code, gAddress, mtAddress)
	if waitingAddress[gAddress] then
		waitingAddress[gAddress](mtAddress)
		waitingAddress[gAddress] = nil
	end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	-- Attempt to call a handler function to further process the packet
	if handler[code] ~= nil then
		handler[code](eventName, receivingModem, sendingModem, port, distance, code, ...)
	end
	-- Just to make it clear, the return value system was a bad idea.
end

local function addressResolver(gAddress)
	local response
	addWaitingAddress(gAddress, function (a) response = a end)
	return waitWithCancel(5, function () return response end)
end

-- Begin startup ---------------------------------------------------------------------------------------------------------------------------
-- transmit broadcast to check for neighboring GERTi enabled computers
if tunnel then
	tunnel.send("AddNeighbor")
end

if modem then
	modem.broadcast(4378, "AddNeighbor")
end

-- Override computer.shutdown to allow for better network leaves
-- (Do this now, not later, in case we somehow are told to shutdown during sleep.
--  It's also right by AddNeighbor which gives a better view of what's going on here.)
local function safedown()
	if tunnel then
		tunnel.send("RemoveNeighbor", modem.address)
	end
	if modem then
		modem.broadcast(4378, "RemoveNeighbor", modem.address)
	end
end
event.listen("shutdown", safedown)

-- Since we're using receivePacket *anyway*, just hook ourselves in and do an os.sleep
-- Register event listener to receive packets from now on
event.listen("modem_message", receivePacket)

-- Wait a while to build the neighbor table.
-- What happens here is that we get RETURNSTART messages, which we don't forward.
-- Completely new neighbors (anything that'll appear after this is over) send AddNeighbor messages,
--  which we then forward. It's important this is high enough for all responses to be collated.
-- The address numbers don't actually matter at all yet
os.sleep(5)

-- forward neighbor table up the line
local serialTable = serialize.serialize(neighbors)
local mncUnavailable = true
if serialTable ~= "{}" then
	-- Even if there is no neighbor table, still register to try and form a network regardless
	local addr = (modem or tunnel).address
	transmitInformation(neighbors[1]["address"], neighbors[1]["port"], "RegisterNode", addr, tier, serialTable)
	addTempHandler(5, "RegisterComplete", function (eventName, recv, sender, port, distance, code, targetMA, iResponse)
		if targetMA == addr then
			iAddress = iResponse
			return true
		end
	end, function () end)
	if waitWithCancel(5, function () return iAddress end) then
		mncUnavailable = false
	end
end
if mncUnavailable then
	print("Unable to contact the MNC. The network will be completely useless.")
end

-- startup procedure is now complete ------------------------------------------------------------------------------------------------------------

-- begin procedure to allow for data transmission
-- this function allows a connection to the requested destination device, should only be used publicly if low-level operation is desired (e.g. another protocol that sits on top of GERTi to further manage networking)
-- (Additional note: The parameter "outbound" is actually the original GERT string-address, this is used for GERTe access.
--                   For GERTe access, destination should be the MNC.)
function GERTi.openRoute(destination, outbound)
	local connectNum = 0
	local isNeighbor = false
	local neighborKey = 0
	local isOpen = false
	local resultGiven = false
	-- attempt to see if neighbor is local
	for key, value in pairs(neighbors) do
		if value["address"] == destination then
			isNeighbor = true
			neighborKey = key
			break
		end
	end
	local function cb(r)
		isOpen = r
		resultGiven = true
	end
	-- if neighbor is local, then open a direct connection
	if isNeighbor == true then
		orController(neighbors[neighborKey]["address"], modem.address, modem.address, neighbors[neighborKey]["address"], neighbors[neighborKey]["port"], neighbors[neighborKey]["port"], cb, outbound)
	else
		-- if neighbor is not local, then attempt to contact parent to open an indirect connection (i.e. routed through multiple computers)
		orController(destination, modem.address, modem.address, neighbors[1]["address"], neighbors[1]["port"], neighbors[1]["port"], cb, outbound)
	end
	waitWithCancel(10, function () return resultGiven end)
	return isOpen
end

-- Yet another low-level function
function GERTi.resolveAddress(gAddress)
	-- attempt to check cached addresses, otherwise contact MNC for address resolution
	for key, value in pairs(cachedAddress) do
		if value["gAddress"] == gAddress then
			return value["realAddress"]
		end
	end
	if realAddress == nil then
		local payload = addressResolver(gAddress)
		if payload then
			cacheAddress(gAddress, payload)
			return payload
		else
			return nil, "invalid address"
		end
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
	return transmitInformation(connections[self.outgoingRoute]["nextHop"], self.outPort, "DATA", data, self.destination, modem.address, self.outbound)
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
	local realAddress, err = GERTi.resolveAddress(gAddress)
	local origination = (modem or tunnel).address
	local incomingRoute = 0
	local outgoingRoute = 0
	local outgoingPort = 0
	local incomingPort = 0
	local isValid = true
	local socket = {}
	if not realAddress then
		return nil, err
	end
	outgoingRoute, incomingRoute, outgoingPort, incomingPort = getConnectionPair(origination, realAddress)
	if incomingRoute == 0 or outgoingRoute == 0 then
		isValid = GERTi.openRoute(realAddress, gAddress)
		if isValid == true then
			outgoingRoute, incomingRoute, outgoingPort, incomingPort = getConnectionPair(origination, realAddress)
		end
	end
	if isValid == true then
		socket.origin = origination
		socket.destination = realAddress
		socket.outbound = gAddress
		socket.incomingRoute = incomingRoute
		socket.outgoingRoute = outgoingRoute
		socket.outPort = outgoingPort
		socket.inPort = incomingPort
		socket.write = writeData
		socket.read = readData
	else
		return nil, "Route cannot be opened, please confirm destination and that a valid path exists."
	end
	return socket
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
