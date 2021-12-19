-- GERTi Client v1.5.1 Build 2
local GERTi = {}
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local serialize = require("serialization")
local mTable, tTable
local iAdd
local tier = 1000
-- nodes[GERTi]{"add", "port", "tier"}, "add" is modem
local nodes = {}
local firstN = {}

-- Connections are established at any point along a route
local connections = {}
local cPend = {}
local rPend = {}

-- Modules are optional add-ons to basic GERTi functionality to extend the network in a transparent manner
local modules = {}
local MNCSocket, DNSSocket
local DNSCache = {}
local hostname
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
		connections[connectDex]["data"]=(connections[connectDex]["data"] or {})
		connections[connectDex]["order"]=1
	end
end
local function storeData(connectDex, order, ...)
	local data = table.pack(...)
	data["n"]=nil
	if #data == 1 then
		data = data[1]
	end
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
handler.Data = function (_, _, port, connectDex, order, ...)
	if connectDex == -1 then
		local dTable = table.pack(...)
		return computer.pushSignal("GERTData", dTable[2], -1, dTable[1])
	end
	if connections[connectDex] then
		if connections[connectDex]["dest"] == iAdd then
			storeData(connectDex, order, ...)
		else
			transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "Data", connectDex, order, ...)
		end
	else
		local origin = string.sub(connectDex, 1, string.find(connectDex, "|")-1)
		local dest = string.sub(connectDex, #origin+2, string.find(connectDex, "|", #origin+2)-1)
		local ID = string.sub(connectDex, #origin+#dest+3)
		origin = tonumber(origin)
		dest = tonumber(dest)
		ID = tonumber(ID)
		if dest == iAdd then
			storeConnection(origin, ID, dest)
			handler.Data(_, _, port, connectDex, order, ...)
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
		transInfo(bHop, receiveM, recPort, "RouteOpen", dest, origin, math.floor(ID))
	end
end
handler.OpenRoute = function (receiveM, sendM, port, dest, intermediary, origin, ID)
	ID = math.floor(ID)
	if cPend[dest.."|"..origin.."|"..ID] then
		local nextHop = tonumber(string.sub(intermediary, 1, string.find(intermediary, "|")-1))
		intermediary = string.sub(intermediary, string.find(intermediary, "|")+1)
		return transInfo(nodes[nextHop]["add"], nodes[nextHop]["receiveM"], nodes[nextHop]["port"], "OpenRoute", dest, intermediary, origin, math.floor(ID))
	elseif dest == iAdd then
		storeConnection(origin, ID, dest)
		return sendOK(sendM, receiveM, port, dest, origin, ID)
	elseif nodes[dest] then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "OpenRoute", dest, nil, origin, math.floor(ID))
	elseif not intermediary then
		transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "OpenRoute", dest, nil, origin, math.floor(ID))
	else
		local nextHop = tonumber(string.sub(intermediary, 1, string.find(intermediary, "|")-1))
		intermediary = string.sub(intermediary, string.find(intermediary, "|")+1)
		transInfo(nodes[nextHop]["add"], nodes[nextHop]["receiveM"], nodes[nextHop]["port"], "OpenRoute", dest, intermediary, origin, math.floor(ID))
	end
	cPend[dest.."|"..origin.."|"..ID]={["bHop"]=sendM, ["port"]=port, ["receiveM"]=receiveM}
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
	if nodes[origin] then
		nodes[origin] = nil
	end
	if noQ then
		transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RemoveNeighbor", origin, noQ)
	end
end
handler.RouteOpen = function (receiveM, sendM, port, dest, origin, ID)
	local cDex = dest.."|"..origin.."|"..math.floor(ID)
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
-- API Functions
local function writeData(self, ...)
	for key, value in pairs(table.pack(...)) do
		if type(value) == "table" or type(value)== "function" then
			return
		end
	end
	if self.destination == iAdd then
		storeData(self.inDex, self.order, ...)
	else
		transInfo(self.nextHop, self.receiveM, self.outPort, "Data", self.outDex, self.order, ...)
	end

	self.order=self.order+1
end
local function readData(self, flags)
	if connections[self.inDex] and connections[self.inDex]["data"][1] ~= nil then
		local data = connections[self.inDex]["data"]
		flags = flags or ""
		if not string.find(flags, "-k") then
			connections[self.inDex]["data"] = {}
		end
		if string.find(flags, "-f") then
			local flatTable = {}
			for key, value in pairs(connections[self.inDex]["data"]) do
				if type(value) == "table" then
					for key2, value2 in pairs(value) do
						table.insert(flatTable, value2)
					end
				else
					table.insert(flatTable, value)
				end
			end
		return flatTable
		end
		return data
	else
		return {}
	end
end
local function closeSock(self)
	handler.CloseConnection(_, _, 4378, self.outDex)
end

function GERTi.openSocket(gAddress, outID)
	local port, add, receiveM
	if (type(gAddress) == "string" and not tonumber(gAddress)) or (math.floor(gAddress) == gAddress and gAddress ~= 0) then
		gAddress = tonumber(DNSCache[gAddress]) or tonumber(GERTi.resolveDNS(gAddress))
	elseif gAddress == 0 then
		gAddress = "0.0"
	else
		gAddress = tonumber(gAddress)
	end
	if not gAddress then
		return false
	end
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
	waitWithCancel(3, function () return (not cPend[gAddress.."|"..iAdd.."|"..outID]) end)
	if not cPend[gAddress.."|"..iAdd.."|"..outID] then
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
		cPend[gAddress.."|"..iAdd.."|"..outID] = nil
		return false
	end
end
function GERTi.broadcast(data)
	if mTable and (type(data) ~= "table" and type(data) ~= "function") then
		component.modem.broadcast(4378, "Data", -1, 0, data, iAdd)
	end
end
function GERTi.send(dest, data)
	if nodes[dest] and (type(data) ~= "table" and type(data) ~= "function") then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "Data", -1, 0, data, iAdd)
	end
end

function GERTi.getAddress()
	return iAdd
end
function GERTi.getAllServices()
	MNCSocket:write("List Services")
	waitWithCancel(3, function () return (#MNCSocket:read("-k")>=1) end)
	local networkServices = serialize.unserialize(MNCSocket:read()[1])
	for key, value in networkServices do
		if not modules[key] then
			modules[key] = value
		end
	end
	return modules
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
function GERTi.getDNSCache()
	return DNSCache
end
function GERTi.getEdition()
	return "BasicClient"
end
function GERTi.getHostname()
	return hostname
end
function GERTi.getLoadedModules()
	return modules
end
function GERTi.getNeighbors()
	return nodes
end
function GERTi.getVersion()
	return "v1.5.1", "1.5.1 Build 1"
end
function GERTi.flushDNS()
	DNSCache = {}
end
function GERTi.isServicePresent(name)
	MNCSocket:write("Get Service Port", name)
	waitWithCancel(3, function () return (#MNCSocket:read("-k")>=1) end)
	return MNCSocket:read()
end
function GERTi.registerModule(modulename, port)
	modules[modulename] = port
end
function GERTi.removeDNSRecord(hostname)
	if DNSSocket then
		DNSSocket:write("Remove Name", hostname, iAdd)
	end
end
function GERTi.resolveDNS(remoteHost)
	if modules["DNS"] then
		DNSSocket:write("DNSResolve", remoteHost)
		waitWithCancel(3, function () return (#DNSSocket:read("-k")>=1) end)
		if #DNSSocket:read("-k")>=1 then
			DNSCache[remoteHost] = DNSSocket:read()[1]
			return DNSCache[remoteHost]
		else
			return nil 
		end
	else
		return nil
	end
end
function GERTi.updateDNSRecord(hostname)
	if DNSSocket then
		DNSSocket:write("Register Name", hostname, iAdd)
	end
end
-- Startup Procedure
event.listen("modem_message", receivePacket)
if computer.getArchitecture()=="Lua 5.2" then
	print("Warning! Lua 5.2 is not supported with GERTi! Please switch the CPU to Lua 5.3 by holding it and right clicking. The Lua architecture can be validated by going to the Lua interpreter and reading the copyright")
end
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
local serialTable = serialize.serialize(nodes)
if serialTable ~= "{}" then
	transInfo(firstN["add"], firstN["receiveM"], firstN["port"], "RegisterNode", firstN["receiveM"], tier, serialTable)
	waitWithCancel(5, function () return iAdd end)
	if not iAdd then
		io.stderr:write("Unable to contact the MNC. Functionality will be impaired.")
		event.ignore("modem_message", receivePacket)
		return
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
event.listen("shutdown", safedown)
MNCSocket = GERTi.openSocket(0.0, 500)
waitWithCancel(3, function () return (connections[MNCSocket.inDex]) end)
if GERTi.isServicePresent("DNS")[1] == 53 then
	DNSSocket = GERTi.openSocket(0.0, 53)
	waitWithCancel(3, function () return (connections[DNSSocket.inDex]) end)
	if filesystem.exists("/etc/hostname") then
		local file = io.open("/etc/hostname", "r")
		hostname = file:read("*l")
	elseif filesystem.exists("/etc/GClient.cfg") then
		local file = io.open("/etc/GClient.cfg")
		hostname = file:read("*l")
	end
	if hostname then
		DNSSocket:write("Register Name", hostname, iAdd)
		GERTi.registerModule("DNS", 53)
	end
end
-- End Startup
return GERTi