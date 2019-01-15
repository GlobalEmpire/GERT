-- GERT v1.1 RC3
local GERTi = {}
local component = require("component")
local computer = require("computer")
local event = require("event")
local serialize = require("serialization")
local modem = nil
local tunnel = nil

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
if not (modem or tunnel) then
	io.stderr:write("This program requires a network or linked card to run.")
	os.exit(1)
end

local iAddress = nil
local tier = 3
-- nodes[GERTi]{"add", "port", "tier"}, "add" is modem
local nodes = {}
local firstN = {["tier"]=4}

-- connections[destinationGAddress][origin][ID]{data} Connections are established at endpoints
local connections = {}
-- paths[origination][destination]{"nextHop", "port"}
local paths = {}

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
	local now = computer.uptime()
	local deadline = now + timeout
	while now < deadline do
		event.pull(deadline - now, "modem_message")
		local response = cancelCheck()
		if response then return response end
		now = computer.uptime()
	end
	return cancelCheck()
end

local function storeNodes(gAddress, sendingModem, port, nTier)
	nodes[gAddress] = {add = sendingModem, port = tonumber(port), tier = nTier}
	if nTier < firstN["tier"] then
		tier = nTier+1
		firstN = nodes[gAddress]
		firstN["gAdd"] = gAddress
	end
end

local function storeConnection(origin, ID, GAdd)
	if not connections[GAdd] then
		connections[GAdd] = {}
	end
	if not connections[GAdd][origin] then
		connections[GAdd][origin] = {}
	end
	connections[GAdd][origin][ID] = {}
end
local function storePath(origin, dest, nextHop, port)
	if paths[origin] == nil then
		paths[origin] = {}
	end
	paths[origin][dest] = {nextHop = nextHop, port = port}
end
local function storeData(origin, ID, data, order)
	if #connections[iAddress][origin][ID] > 20 then
		table.remove(connections[iAddress][origin][ID], 1)
	end
	table.insert(connections[iAddress][origin][ID], data)
	computer.pushSignal("GERTData", origin, ID)
end

local function transInfo(sendTo, port, ...)
	if modem and port ~= 0 then
		modem.send(sendTo, port, ...)
	elseif tunnel then
		tunnel.send(...)
	end
end

local handler = {}
handler.CloseConnection = function(sendingModem, port, ID, dest, origin)
	if destination ~= iAddress then
		transInfo(paths[origin][dest]["nextHop"], paths[origin][dest]["port"], "CloseConnection", ID, dest, origin)
	end
	paths[origin][dest] = nil
	connections[dest][origin][ID] = nil
end

handler.Data = function (sendingModem, port, data, dest, origin, ID, order)
	if ID < 0 then
		return computer.pushSignal("GERTData", origin, ID, data)
	end
	if connections[dest][origin][ID] then
		storeData(origin, ID, data, order)
	else
		transInfo(paths[origin][dest]["nextHop"], paths[origin][dest]["port"], "Data", data, dest, origin, ID, order)
	end
end

handler.NewNode = function (sendingModem, port)
	transInfo(sendingModem, port, "RETURNSTART", iAddress, tier)
end

local function routeOpener(dest, origin, bHop, nextHop, recPort, transPort, ID)
	local function sendOKResponse(isDestination)
		transInfo(bHop, recPort, "RouteOpen", dest, origin)
		if isDestination then
			storePath(origin, dest, nextHop, transPort)
			storeConnection(origin, ID, dest)
			computer.pushSignal("GERTConnectionID", origin, ID)
		else
			storePath(origin, dest, nextHop, transPort)
		end
	end
	if iAddress ~= dest then
		local response
		transInfo(nextHop, transPort, "OpenRoute", dest, "a", origin, ID)
		addTempHandler(3, "RouteOpen", function (eventName, recv, sender, port, distance, code, pktDest, pktOrig)
			if (dest == pktDest) and (origin == pktOrig) then
				response = code
				sendOKResponse(false)
				return true -- This terminates the wait
			end
		end, function () end)
		waitWithCancel(3, function () return response end)
	else
		sendOKResponse(true)
	end
end

handler.OpenRoute = function (sendingModem, port, dest, intermediary, origin, ID)
	-- Is destination this computer?
	if dest == iAddress then
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

handler.RemoveNeighbor = function (sendingModem, port, origination)
	if nodes[origination] ~= nil then
		nodes[origination] = nil
	end
	transInfo(firstN["add"], firstN["port"], "RemoveNeighbor", origination)
end

handler.RegisterNode = function (sendingModem, sendingPort, origination, nTier, serialTable)
	transInfo(firstN["add"], firstN["port"], "RegisterNode", origination, nTier, serialTable)
	addTempHandler(3, "RegisterComplete", function (eventName, recv, sender, port, distance, code, targetMA, iResponse)
		if targetMA == origination then
			transInfo(sendingModem, sendingPort, "RegisterComplete", targetMA, iResponse)
			return true
		end
	end, function () end)
end

handler.RETURNSTART = function (sendingModem, port, gAddress, nTier)
	storeNodes(tonumber(gAddress), sendingModem, port, nTier)
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
os.sleep(2)

-- forward neighbor table up the line
local serialTable = serialize.serialize(nodes)
if serialTable ~= "{}" then
	local mncUnavailable = true
	local addr = (modem or tunnel).address
	transInfo(firstN["add"], firstN["port"], "RegisterNode", addr, tier, serialTable)
	addTempHandler(3, "RegisterComplete", function (_, _, _, _, _, code, targetMA, iResponse)
		if targetMA == addr then
			iAddress = tonumber(iResponse)
			mncUnavailable = false
			return true
		end
	end, function () end)
	waitWithCancel(5, function () return iAddress end)
	if mncUnavailable then
		print("Unable to contact the MNC. Functionality will be impaired.")
	end
end

if tunnel then
	tunnel.send("RETURNSTART", iAddress, tier)
end
if modem then
	modem.broadcast(4378, "RETURNSTART", iAddress, tier)
end
--Listen to computer.shutdown to allow for better network leaves
local function safedown()
	if tunnel then
		tunnel.send("RemoveNeighbor", iAddress)
	end
	if modem then
		modem.broadcast(4378, "RemoveNeighbor", iAddress)
	end
	for key, value in pairs(connections) do
		for key2, value2 in pairs(connections[key]) do
			for key3, value3 in pairs(connections[key][key2]) do
				handler.CloseConnection((modem or tunnel).address, 4378, key3, key, key2)
			end
		end
	end
end
event.listen("shutdown", safedown)

-------------------
local function writeData(self, data)
	if type(data) ~= "table" or type(data) ~= "function" then
		transInfo(self.nextHop, self.outPort, "Data", data, self.destination, self.origination, self.ID, self.order)
		self.order=self.order+1
	end
end

local function readData(self, doPeek)
	if connections[iAddress] and connections[iAddress][self.destination] and connections[iAddress][self.destination][self.ID] then
		local data = connections[iAddress][self.destination][self.ID]
		if tonumber(doPeek) ~= 2 then
			connections[iAddress][self.destination][self.ID] = {}
		end
		return data
	else
		return {}
	end
end

local function closeSock(self)
	transInfo(self.nextHop, self.outPort, "CloseConnection", self.ID, self.destination, self.origination)
	handler.CloseConnection((modem or tunnel).address, 4378, self.ID, self.destination, self.origination)
end
function GERTi.openSocket(gAddress, doEvent, outID)
	if outID == nil then
		if connections[gAddress] and connections[gAddress][iAddress] then
			outID = #connections[gAddress][iAddress] + 1
		else
			outID = 1
		end
	end
	if nodes[gAddress] ~= nil then
		storeConnection(iAddress, outID, gAddress)
		routeOpener(gAddress, iAddress, "A", nodes[gAddress]["add"], nodes[gAddress]["port"], nodes[gAddress]["port"], outID)
	else
		storeConnection(iAddress, outID, gAddress)
		routeOpener(gAddress, iAddress, "A", firstN["add"], firstN["port"], firstN["port"], outID)
	end
	
	local socket = {origination = iAddress,
		destination = gAddress,
		outPort = nodes[gAddress]["port"] or firstN["port"],
		nextHop = nodes[gAddress]["add"] or firstN["add"],
		ID = outID,
		order = 1,
		write = writeData,
		read = readData,
		close = closeSock}
	return socket
end
function GERTi.send(dest, data)
	if nodes[dest] and (type(data) ~= "table" or type(data) ~= "function") then
		transInfo(nodes[dest]["add"], nodes[dest]["port"], "Data", data, dest, iAddress, -1)
	end
end
function GERTi.getConnections()
	local tempTable = {}
	for key, value in pairs(connections) do
		tempTable[key] = {}
		for key2, value2 in pairs(connections[key]) do
			tempString = ""
			for key3, value3 in pairs(connections[key][key2]) do
				tempString = tempString..key3..","
			end
			tempTable[key][key2]=tempString
		end
	end
	return tempTable
end

function GERTi.getNeighbors()
	return nodes
end

function GERTi.getAddress()
	return iAddress
end
return GERTi