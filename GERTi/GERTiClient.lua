-- GERT v1.4.1
local GERTi = {}
local component = require("component")
local computer = require("computer")
local event = require("event")
local serialize = require("serialization")
local mTable, tTable

if (component.isAvailable("modem")) then
	mTable = {}
	for address, value in component.list("modem") do
		mTable[address] = component.proxy(address)
		mTable[address].open(4378)
		if (mTable[address].isWireless()) then
			mTable[address].setStrength(500)
		end
	end
end
if (component.isAvailable("tunnel")) then
	tTable = {}
	for address, value in component.list("tunnel") do
		tTable[address] = component.proxy(address)
	end
end
if not (mTable or tTable) then
	io.stderr:write("This program requires a network card or linked card to run.")
	os.exit(1)
end

local iAdd
local tier = 3
-- nodes[GERTi]{"add", "port", "tier"}, "add" is modem
local nodes = {}
local firstN = {}

-- Connections are established at any point along a route
local connections = {}
local cPend = {}
local rPend = {}
local function waitWithCancel(timeout, cancelCheck)
	local now = computer.uptime()
	local deadline = now + timeout
	while now < deadline do
		event.pull(deadline - now, "modem_message")
		local response = cancelCheck()
		if response then return end
		now = computer.uptime()
	end
end

local function storeNodes(gAddress, receiverM, sendM, port, nTier)
	nodes[gAddress] = {add = sendM, receiveM = receiverM, port = tonumber(port), tier = nTier}
	if (not firstN["tier"]) or nTier < firstN["tier"] then
		tier = nTier+1
		firstN = nodes[gAddress]
		firstN["gAdd"] = gAddress
	end
end
local function storeConnection(origin, ID, GAdd, nextHop, sendM, port)
	ID = math.floor(ID)
	local connectDex = origin.."|"..GAdd.."|"..ID
	connections[connectDex] = {["origin"]=origin, ["dest"]=GAdd, ["ID"]=ID}
	if GAdd ~= iAdd then
		connections[connectDex]["nextHop"]=nextHop
		connections[connectDex]["sendM"] = sendM
		connections[connectDex]["port"]=port
	else
		connections[connectDex]["data"]={}
		connections[connectDex]["order"]=1
	end
end

local function storeData(connectDex, data, order)
	if #connections[connectDex]["data"] > 20 then
		table.remove(connections[connectDex]["data"], 1)
	end
	if order >= connections[connectDex]["order"] then
		table.insert(connections[connectDex]["data"], data)
		connections[connectDex]["order"] = order
	else
		table.insert(connections[connectDex]["data"], #connections[connectDex]["data"], data)
	end
	computer.pushSignal("GERTData", connections[connectDex]["origin"], connections[connectDex]["ID"])
end

local function transInfo(sendTo, localM, port, ...)
	if mTable and mTable[localM] then
		mTable[localM].send(sendTo, port, ...)
	elseif tTable and tTable[localM] then
		tTable[localM].send(...)
	end
end

local handler = {}
handler.CloseConnection = function(_, _, port, connectDex)
	if connections[connectDex]["nextHop"] then
		transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "CloseConnection", tostring(connectDex))
	else
		computer.pushSignal("GERTConnectionClose", connections[connectDex]["origin"], connections[connectDex]["dest"], connections[connectDex]["ID"])
	end
	connections[connectDex] = nil
end

handler.Data = function (_, _, port, data, connectDex, order, origin)
	if connectDex == -1 then
		return computer.pushSignal("GERTData", origin, -1, data)
	end
	if connections[connectDex] then
		if connections[connectDex]["dest"] == iAdd then
			storeData(connectDex, data, order)
		else
			transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "Data", data, connectDex, order)
		end
	else
		origin = string.sub(connectDex, 1, string.find(connectDex, "|")-1)
		local dest = string.sub(connectDex, #origin+2, string.find(connectDex, "|", #origin+2)-1)
		local ID = string.sub(connectDex, #origin+#dest+3)
		origin = tonumber(origin)
		dest = tonumber(dest)
		ID = tonumber(ID)
		if dest == iAdd then
			storeConnection(origin, ID, dest)
			handler.Data(_, _, port, data, connectDex, order)
		end
	end
end

handler.NewNode = function (receiveM, sendM, port, gAddress, nTier)
	if gAddress then
		storeNodes(tonumber(gAddress), receiveM, sendM, port, nTier)
	else
		transInfo(sendM, receiveM, port, "NewNode", iAdd, tier)
	end
end
local function sendOK(bHop, receiveM, recPort, dest, origin, ID)
	if dest==iAdd then
		computer.pushSignal("GERTConnectionID", origin, ID)
	end
	if origin ~= iAdd then
		transInfo(bHop, receiveM, recPort, "RouteOpen", dest, origin, tostring(ID))
	end
end
handler.OpenRoute = function (receiveM, sendM, port, dest, intermediary, origin, ID)
	if cPend[dest..origin..ID] then
		local nextHop = tonumber(string.sub(intermediary, 1, string.find(intermediary, "|")-1))
		intermediary = string.sub(intermediary, string.find(intermediary, "|")+1)
		return transInfo(nodes[nextHop]["add"], nodes[nextHop]["receiveM"], nodes[nextHop]["port"], "OpenRoute", dest, intermediary, origin, tostring(ID))
	elseif dest == iAdd then
		storeConnection(origin, ID, dest)
		return sendOK(sendM, receiveM, port, dest, origin, ID)
	elseif nodes[dest] then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "OpenRoute", dest, nil, origin, tostring(ID))
	elseif not intermediary then
		transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "OpenRoute", dest, nil, origin, tostring(ID))
	else
		local nextHop = tonumber(string.sub(intermediary, 1, string.find(intermediary, "|")-1))
		intermediary = string.sub(intermediary, string.find(intermediary, "|")+1)
		transInfo(nodes[nextHop]["add"], nodes[nextHop]["receiveM"], nodes[nextHop]["port"], "OpenRoute", dest, intermediary, origin, tostring(ID))
	end
	cPend[dest..origin..ID]={["bHop"]=sendM, ["port"]=port, ["receiveM"]=receiveM}
end
handler.RegisterComplete = function(receiveM, _, port, target, newG)
	if (mTable and mTable[target]) or (tTable and tTable[target]) then
		iAdd = tonumber(newG)
	elseif rPend[target] then
		transInfo(rPend[target]["add"], rPend[target]["receiveM"], rPend[target]["port"], "RegisterComplete", target, newG)
		rPend[target] = nil
	end
end
handler.RegisterNode = function (receiveM, sendM, sPort, origin, nTier, serialTable)
	transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RegisterNode", origin, nTier, serialTable)
	rPend[origin] = {["receiveM"]=receiveM, ["add"]=sendM, ["port"] = sPort}
end

handler.RemoveNeighbor = function (receiveM, _, port, origin, noQ)
	if nodes[origination] then
		nodes[origination] = nil
	end
	if noQ then
		transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RemoveNeighbor", origin, noQ)
	end
end

handler.RouteOpen = function (receiveM, sendM, port, dest, origin, ID)
	local cDex = dest..origin..ID
	if cPend[cDex] then
		sendOK(cPend[cDex]["bHop"], cPend[cDex]["receiveM"], cPend[cDex]["port"], dest, origin, ID)
		storeConnection(origin, ID, dest, sendM, receiveM, port)
		cPend[cDex] = nil
	end
end

local function receivePacket(_, receiveM, sendM, port, distance, code, ...)
	if handler[code] then
		handler[code](receiveM, sendM, port, ...)
	end
end

------------------------------------------
event.listen("modem_message", receivePacket)
if tTable then
	for key, value in pairs(tTable) do
		tTable[key].send("NewNode")
	end
end
if mTable then
	for key, value in pairs(mTable) do
		mTable[key].broadcast(4378, "NewNode")
	end
end
os.sleep(2)

-- forward neighbor table up the line
local serialTable = serialize.serialize(nodes)
if serialTable ~= "{}" then
	local mncUnavailable = true
	local addr
	transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RegisterNode", firstN["receiveM"], tier, serialTable)
	waitWithCancel(5, function () return iAdd end)
	if not iAdd then
		print("Unable to contact the MNC. Functionality will be impaired.")
	end
end

if tTable then
	for key, value in pairs(tTable) do
		tTable[key].send("NewNode", iAdd, tier)
	end
end
if mTable then
	for key, value in pairs(mTable) do
		mTable[key].broadcast(4378, "NewNode", iAdd, tier)
	end
end

--Listen to computer.shutdown to allow for better network leaves
local function safedown()
	for key, value in pairs(connections) do
		handler.CloseConnection(_, _, 4378, key)
	end
	if tTable then
		for key, value in pairs(tTable) do
			tTable[key].send("RemoveNeighbor", iAdd)
		end
	end
	if mTable then
		for key, value in pairs(mTable) do
			mTable[key].broadcast(4378, "RemoveNeighbor", iAdd)
		end
	end
	transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RemoveNeighbor", iAdd, 1)
end
event.listen("shutdown", safedown)

-------------------
local function writeData(self, data)
	if type(data) ~= "table" and type(data) ~= "function" then
		transInfo(self.nextHop, self.receiveM, self.outPort, "Data", data, self.outDex, self.order)
		self.order=self.order+1
	end
end

local function readData(self, doPeek)
	if connections[self.inDex] then
		local data = connections[self.inDex]["data"]
		if tonumber(doPeek) ~= 2 then
			connections[self.inDex]["data"] = {}
		end
		return data
	else
		return {}
	end
end

local function closeSock(self)
	handler.CloseConnection(_, _, 4378, self.outDex)
end
function GERTi.openSocket(gAddress, doEvent, outID)
	if type(doEvent) ~= "boolean" then
		outID = doEvent
	end
	local port, add, receiveM
	if not outID then
		outID = #connections + 1
	end
	outID = math.floor(outID)
	if nodes[gAddress] then
		port = nodes[gAddress]["port"]
		add = nodes[gAddress]["add"]
		receiveM = nodes[gAddress]["receiveM"]
		handler.OpenRoute(receiveM, receiveM, 4378, gAddress, nil, iAdd, outID)
	else
		handler.OpenRoute(firstN["receiveM"], firstN["receiveM"], 4378, gAddress, nil, iAdd, outID)
	end
	waitWithCancel(3, function () return (not cPend[gAddress..iAdd..outID]) end)
	if not cPend[gAddress..iAdd..outID] then
		local socket = {origination = iAdd,
			destination = gAddress,
			outPort = port or firstN["port"],
			nextHop = add or firstN["add"],
			receiveM = receiveM or firstN["receiveM"],
			ID = outID,
			order = 1,
			outDex=iAdd.."|"..gAddress.."|"..outID,
			inDex = gAddress.."|"..iAdd.."|"..outID,
			write = writeData,
			read = readData,
			close = closeSock}
		return socket
	else
		return false
	end
end
function GERTi.broadcast(data)
	if mTable and (type(data) ~= "table" and type(data) ~= "function") then
		component.modem.broadcast(4378, "Data", data, -1, 0, iAdd)
	end
end
function GERTi.send(dest, data)
	if nodes[dest] and (type(data) ~= "table" and type(data) ~= "function") then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "Data", data, -1, 0, iAdd)
	end
end
function GERTi.getConnections()
	local tempTable = {}
	for key, value in pairs(connections) do
		tempTable[key] = {}
		tempTable[key]["origin"] = value["origin"]
		tempTable[key]["destination"] = value["dest"]
		tempTable[key]["ID"] = value["ID"]
		tempTable[key]["nextHop"] = value["nextHop"]
		tempTable[key]["receiveM"] = value["receiveM"]
		tempTable[key]["port"] = value["port"]
		tempTable[key]["order"] = value["order"]
	end
	return tempTable
end

function GERTi.getNeighbors()
	return nodes
end

function GERTi.getAddress()
	return iAdd
end
function GERTi.getVersion()
	return "v1.4.1", "1.4.1"
end
return GERTi