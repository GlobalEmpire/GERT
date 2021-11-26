-- MNC API v1.5 Build 12
local MNCAPI = {}
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local serialize = require("serialization")
local mTable, tTable, nodes, connections, cPend
local iAdd = 0.0
local tier = 0
local sockets = {}
local networkServices = {}
local DNSCache = {}
local DNSSocket
local modules = {}
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
		if response then return response end
		now = computer.uptime()
	end
	return cancelCheck()
end

local function transInfo(sendTo, localM, port, ...)
	if mTable and mTable[localM] then
		mTable[localM].send(sendTo, port, ...)
	elseif tTable and tTable[localM] then
		tTable[localM].send(...)
	end
end

local function writeData(self, ...)
	for key, value in pairs(table.pack(...)) do
		if type(value) == "table" or type(value)== "function" then
			return
		end
	end
	if self.destination == iAdd then
		computer.pushSignal("modem_message", _, _, _, self.inDex, self.order, ...)
	else
		transInfo(self.nextHop, self.receiveM, self.outPort, "Data", self.outDex, self.order, ...)
	end
	self.order=self.order+1
end

local function readData(self, flags)
	if connections[self.inDex] and connections[self.inDex]["data"][1] then
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
	computer.pushSignal("modem_message", _, _, _, _, "CloseConnection", self.outDex)
end

function MNCAPI.openSocket(gAddress, outID)
	local receiveM, cDex
	if type(gAddress) == "string" or (math.floor(gAddress) == gAddress and gAddress ~= 0) then
		gAddress = DNSCache[gAddress] or MNCAPI.resolveDNS(gAddress)
	end
	if not gAddress then
		return false
	end
	if not outID then
		outID = #connections + 1
	end
	outID = math.floor(outID)
	if nodes[gAddress] then
		receiveM = nodes[gAddress]["receiveM"]
		computer.pushSignal("modem_message", receiveM, receiveM, 4378, 0, "OpenRoute", gAddress, nil, iAdd, outID)
		cDex = gAddress.."|"..iAdd.."|"..outID
	else
		return false
	end
	waitWithCancel(3, function () return (not cPend[cDex]) end)
	if not cPend[cDex] then
		cDex = iAdd.."|"..gAddress.."|"..outID
		local socket = {origination = iAdd,
			destination = gAddress,
			outPort = connections[cDex]["port"],
			nextHop = connections[cDex]["nextHop"],
			receiveM = connections[cDex]["sendM"],
			ID = outID,
			order = 1,
			outDex=cDex,
			inDex = gAddress.."|"..iAdd.."|"..outID,
			write = writeData,
			read = readData,
			close = closeSock}
		return socket
	else
		cPend[cDex] = nil
		return false
	end
end
function MNCAPI.broadcast(data)
	if mTable and (type(data) ~= "table" and type(data) ~= "function") then
		component.modem.broadcast(4378, "Data", -1, 0, data, iAdd)
	end
end
function MNCAPI.registerModule(modulename, port)
	modules[modulename] = port
end
function MNCAPI.resolveDNS(remoteHost)
	if modules["DNS"] then
		DNSSocket:write("DNSResolve", remoteHost)
		waitWithCancel(3, function () return (#DNSSocket:read("-k")>=1) end)
		DNSCache[remoteHost] = DNSSocket:read()[1]
		return DNSCache[remoteHost]
	else
		return nil
	end
end
function MNCAPI.send(dest, data)
	if nodes[dest] and (type(data) ~= "table" and type(data) ~= "function") then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "Data", -1, 0, data, iAdd)
	end
end

function MNCAPI.getAddress()
	return iAdd
end
function MNCAPI.getAllServices()
	return modules
end
function MNCAPI.getConnections()
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
function MNCAPI.getDNSCache()
	return DNSCache
end
function MNCAPI.getEdition()
	return "MNCAPI"
end
function MNCAPI.getHostname()
	return "MNC"
end
function MNCAPI.getLoadedModules()
	return modules
end
function MNCAPI.getNeighbors()
	return nodes
end
function MNCAPI.getVersion()
	return "v1.5", "1.5 Build 8"
end
function MNCAPI.isServicePresent(name)
	return modules[name]
end
function MNCAPI.loadTables(nTable, cTable, cPTable)
	nodes = nTable
	connections = cTable
	cPend = cPTable
	if filesystem.exists("/etc/rc.d/DNS.lua") then
		DNSSocket = MNCAPI.openSocket(0.0, 53)
	end
	print("MNCAPI Loaded")
end
function MNCAPI.registerNetworkService(name, port)
	networkServices[name] = port
end
function MNCAPI.removeDNSRecord(hostname)
	if DNSSocket then
		DNSSocket:write("Remove Name", hostname, iAdd)
	end
end
function MNCAPI.updateDNSRecord(hostname, address)
	if DNSSocket then
		DNSSocket:write("Register Name", hostname, address)
	end
end
-- Startup Procedure
local function checkConnection(_, origin, ID)
	if ID == 500 then
		local newSocket = MNCAPI.openSocket(origin, ID)
		sockets[origin] = newSocket
	end
end
event.listen("GERTConnectionID", checkConnection)

local function checkData(_, origin, ID)
	if sockets[origin] and ID == 500 then
		local data = sockets[origin]:read()
		if data[1] == "List Services" then
			sockets[origin]:write(serialize.serialize(networkServices))
		elseif data[1][1] == "Get Service Port" then
			if networkServices[data[1][2]] then
				sockets[origin]:write(networkServices[data[1][2]])
			else
				sockets[origin]:write(false)
			end
		end
	end
end
event.listen("GERTData", checkData)

local function closeSockets(_, origin, dest, ID)
	if ID == 500 and dest == 0.0 then
		sockets[origin]:close()
		sockets[origin] = nil
	end
end
event.listen("GERTConnectionClose", closeSockets)
-- End Startup
return MNCAPI