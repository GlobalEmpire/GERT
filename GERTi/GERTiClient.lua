-- GERT v1.1 Build 1
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

local iAddress = nil
local tier = 3
-- Tables of nodes and connections
-- nodes[GERTi]{"add", "port", "tier"}, with the key being the neighbor's GERTi address, and the "add" field being the modem UUID
local nodes = {}
local firstN = {["tier"]=4}

-- connections[destinationGAddress][ID]{"origination", "data", "doEvent"} Connections are established at endpoints
local connections = {}
-- paths[origination][destination]{"nextHop", "port"}
local paths = {}

-- this function adds a handler for a set time in seconds, or until that handler returns a truthful value (whichever comes first)
local function addTempHandler(timeout, code, cb, cbf)
	local disable = false
	local function cbi(...)
		if disable then return end
		local evn, rc, sd, pt, dt, code2 = ...
		if code ~= code2 then return end
		if cb(...) then
			disable = true
			return false
		end
	end
	event.listen("modem_message", cbi)
	event.timer(timeout, function ()
		event.ignore("modem_message", cbi)
		if disable then return end
		cbf()
	end)
end
-- Like a sleep, but it will exit early if a modem_message is received and then something happens.
local function waitWithCancel(timeout, cancelCheck)
	-- Wait for the response.
	local now = computer.uptime()
	local deadline = now + timeout
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

local function storeNodes(gAddress, sendingModem, port, tier)
	-- Register  for communication to the rest of the network
	nodes[gAddress] = {}
	nodes[gAddress]["add"] = sendingModem
	nodes[gAddress]["port"] = tonumber(port)
	nodes[gAddress]["tier"] = tier
	if tier < firstN then
		firstN = nodes[gAddress]
		firstN["gAdd"] = gAddress
	end
end

local function storeConnection(origination, doEvent, connectionID, GAdd)
	if connections[GAdd] == nil then
		connections[GAdd] = {}
	end
	connections[GAdd][connectionID] = {}
	connections[GAdd][connectionID]["origination"] = origination
	connections[GAdd][connectionID]["destination"] = GAdd
	connections[GAdd][connectionID]["data"] = {}
	connections[GAdd][connectionID]["connectionID"] = connectionID
	connections[GAdd][connectionID]["doEvent"] = (doEvent or false)
end
local function storePath(origination, destination, nextHop, port)
	if paths[origination] == nil then
		paths[origination] = {}
	end
	paths[origination][destination] = {}
	paths[pathDex]["nextHop"] = nextHop
	paths[pathDex]["port"] = port
end
-- Stores data inside a connection for use by a program
local function storeData(ID, data)
	if #connections[iAddress][ID]["data"] >= 20 then
		table.remove(connections[iAddress][ID]["data"], 1)
	end
	table.insert(connections[iAddress][ID]["data"], data)
	if connections[iAddress][ID]["doEvent"] then
		computer.pushSignal("GERTData", connections[iAddress][ID]["origination"], ID)
	end
end

local function transmitInformation(sendTo, port, ...)
	if (port ~= 0) and (modem) then
		modem.send(sendTo, port, ...)
	elseif (tunnel) then
		tunnel.send(...)
	end
end

local handler = {}
handler["CloseConnection"] = function(sendingModem, port, ID, destination, origin)
	transmitInformation(paths[origin][destination]["nextHop"], paths[origin][destination]["port"], "CloseConnection", ID, destination, origin)
	paths[origin][destination] = nil
	connections[destination][ID] = nil
end

handler["Data"] = function (sendingModem, port, data, destination, origination, connectionID)
	-- Attempt to determine if host is the destination, else send it on to next hop.
	if connections[origination] ~= nil and connections[origination][connectionID] ~= nil then
		storeData(connectionID, data)
	else
		transmitInformation(value["nextHop"], value["port"], "Data", data, destination, origination, connectionID)
	end
end

handler["NewNode"] = function (sendingModem, port)
	transmitInformation(sendingModem, port, "RETURNSTART", iAddress, tier)
end

local function routeOpener(dest, origin, bHop, nextHop, recPort, transPort, ID)
	local function sendOKResponse(isDestination)
		transmitInformation(bHop, recPort, "ROUTE OPEN", dest, origin)
		if isDestination then
			storePath(origin, dest, nextHop, transPort)
			storeConnection(origin, false, ID, dest)
			computer.pushSignal("GERTConnectionID", origin, ID)
		else
			storePath(origin, dest, nextHop, transPort)
		end
	end
	if iAddress ~= dest then
		transmitInformation(nextHop, transPort, "OpenRoute", dest, "a", origin, ID)
		addTempHandler(3, "ROUTE OPEN", function (eventName, recv, sender, port, distance, code, pktDest, pktOrig)
			if (destination == pktDest) and (origination == pktOrig) then
				sendOKResponse(false)
				return true -- This terminates the wait
			end
		end, function () end)
		waitWithCancel(3, function () return response end)
	else
		sendOKResponse(true)
	end
end

handler["OpenRoute"] = function (sendingModem, port, dest, intermediary, origin, ID)
	-- Is destination this computer?
	if destination == iAddress then
		return routeOpener(iAddress, origin, sendingModem, (modem or tunnel).address, port, port, ID)
	end
	-- Is destination a neighbor?
	if nodes[dest] then
		return routeOpener(dest, origin, sendingModem, nodes[dest]["add"], port, nodes[dest]["port"], ID)
	end
	-- If no neighbor or intermediary found, forward to MNC
	if not nodes[intermediary] then
		return routeOpener(dest, origin, sendingModem, firstN["add"], port, firstN["port"], ID)
	end
	-- If an intermediary is found, forward to intermediary
	routeOpener(dest, origin, sendingModem, nodes[intermediary]["add"], port, nodes[intermediary]["port"], ID)
end

handler["RemoveNeighbor"] = function (sendingModem, port, origination)
	if nodes[origination] ~= nil then
		nodes[origination] = nil
	end
	transmitInformation(firstN["add"], firstN["port"], "RemoveNeighbor", origination)
end

handler["RegisterNode"] = function (sendingModem, sendingPort, origination, tier, serialTable)
	transmitInformation(firstN["add"], firstN["port"], "RegisterNode", origination, tier, serialTable)
	addTempHandler(3, "RegisterComplete", function (eventName, recv, sender, port, distance, code, targetMA, iResponse)
		if targetMA == origination then
			transmitInformation(sendingModem, sendingPort, "RegisterComplete", targetMA, iResponse)
			return true
		end
	end, function () end)
end

handler["RETURNSTART"] = function (sendingModem, port, gAddress, tier)
	storeNodes(gAddress, sendingModem, port, tier)
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	if handler[code] ~= nil then
		handler[code](sendingModem, port, ...)
	end
end

------------------------------------------
if tunnel then
	tunnel.send("NewNode")
end
if modem then
	modem.broadcast(4378, "NewNode")
end
event.listen("modem_message", receivePacket)

-- Wait a while to build the neighbor table.
os.sleep(2)

-- forward neighbor table up the line
local serialTable = serialize.serialize(nodes)
local mncUnavailable = true
if serialTable ~= "{}" then
	local addr = (modem or tunnel).address
	transmitInformation(firstN["add"], firstN["port"], "RegisterNode", addr, tier, serialTable)
	addTempHandler(3, "RegisterComplete", function (_, _, _, _, _, code, targetMA, iResponse)
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
	print("Unable to contact the MNC. Functionality will be impaired.")
end
if tunnel then
	tunnel.send("RETURNSTART", iAddress, tier)
end
if modem then
	modem.broadcast(4378, "RETURNSTART", iAddress, tier)
end
-- Override computer.shutdown to allow for better network leaves
local function safedown()
	if tunnel then
		tunnel.send("RemoveNeighbor", iAddress)
	end
	if modem then
		modem.broadcast(4378, "RemoveNeighbor", iAddress)
	end
	for key, value in pairs(connections) do
		handler["CloseConnection"]((modem or tunnel).address, 4378, "CloseConnection", value["connectionID"], value["destination"], value["origination"])
	end
end
event.listen("shutdown", safedown)

-------------------
local function writeData(self, data)
	transmitInformation(self.nextHop, self.outPort, "Data", data, self.destination, self.origination, self.ID)
end

local function readData(self, doPeek)
	if connections[iAddress][self.ID] ~= nil then
		if (not connections[iAddress][self.ID]["doEvent"]) and self.doEvent then
			connections[iAddress][self.ID]["doEvent"] = true
		end
		local data = connections[iAddress][self.ID]["data"]
		if tonumber(doPeek) ~= 2 then
			connections[iAddress][self.ID]["data"] = {}
			connections[iAddress][self.ID]["dataDex"] = 1
		end
		return data
	else
		return {}
	end
end

local function closeSock(self)
	transmitInformation(self.nextHop, self.outPort, "CloseConnection", self.ID, self.destination, self.origination)
	handler["CloseConnection"]((modem or tunnel).address, 4378, "CloseConnection", self.ID, self.destination, self.origination)
end
function GERTi.openSocket(gAddress, doEvent, outID)
	if outID == nil then
		outID = #connections[iAddress] + 1
	end
	if nodes[gAddress] ~= nil then
		storeConnection(iAddress, false, outID, gAddress)
		routeOpener(destination, iAddress, iAddress, nodes[gAddress]["add"], nodes[gAddress]["port"], nodes[gAddress]["port"], outID)
	else
		storeConnection(iAddress, false, outID, gAddress)
		routeOpener(destination, iAddress, iAddress, firstN["add"], firstN["port"], firstN["port"], outID)
	end
	
	local socket = {}
	socket.origination = iAddress
	socket.destination = gAddress
	socket.outPort = nodes[gAddress]["port"] or firstN["port"]
	socket.nextHop = nodes[gAddress]["add"] or firstN["add"]
	socket.ID = outID
	socket.write = writeData
	socket.read = readData
	socket.close = closeSock
	socket.doEvent = doEvent
	return socket
end

function GERTi.getConnections()
	local tempTable = {}
	for key, value in pairs(connections) do
		tempTable[key] = {}
		for key2, value2 in pairs(connections[key]) do
			tempTable[key]["ID"]=key2
		end
	end
	return tempTable
end

function GERTi.getNeighbors()
	return nodes
end

function GERTi.getPaths()
	return paths
end

function GERTi.getAddress()
	return iAddress
end
return GERTi