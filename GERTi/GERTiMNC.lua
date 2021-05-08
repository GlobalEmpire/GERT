-- GERT v1.4.1
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local GERTe
local os = require("os")
local serialize = require("serialization")
local mTable, tTable, gAddress, gKey

local nodes = {}
local connections = {}
local cPend = {}
local rPend = {}
local addressP1 = 0
local addressP2 = 1
local timerID ={}
local savedAddresses = {}
local directory = "/etc/GERTaddresses.gert"

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

local function storeChild(rAddress, receiveM, port, tier)
	local childGA
	if savedAddresses[rAddress] then -- check to see if the address is already saved
		childGA = savedAddresses[rAddress]
	end
	if not childGA then
		childGA = addressP1.."."..addressP2
		childGA = tonumber(childGA)
		savedAddresses[rAddress] = childGA
		local f = io.open(directory, "a")
		f:write(addressP1.."."..addressP2.."\n")
		f:write(rAddress.."\n")
		f:close()
		if addressP2 == 4095 then
			addressP2 = 0
			addressP1 = addressP1 + 1
			if addressP1 >= 4096 then
				io.stderr:write("Warning! Maximum number of clients exceeded! Please clean the GERTaddresses.gert file of excess addresses. If error persists, please contact MajGenRelativity at https://discord.gg/f7VYMfJ")
			end
		else
			addressP2 = addressP2 + 1
		end
		if (addressP2 % 10) == 0 then
				addressP2 = addressP2 +1
		end
	end
	nodes[childGA] = {["add"] = rAddress, ["receiveM"] = receiveM, ["tier"] = tonumber(tier), ["port"] = tonumber(port), ["neighbors"]={}} -- Store modem address of the endpoint, modem address of the modem used to contact it, the tier, and the transmission port used to contact the client.
	return childGA
end

local function storeConnection(origin, ID, dest, nextHop, sendM, port) -- GERT address of the connection origin, Connection ID, GERT address of the destination, next node in the connection, modem used to reach the next node, and the port used to reach the next mode
	local connectDex
	ID = math.floor(ID)
	connectDex = origin.."|"..dest.."|"..ID
	connections[connectDex] = {["origin"]=origin, ["dest"]=dest, ["ID"]=ID, ["nextHop"]=nextHop, ["sendM"] = sendM, ["port"]=port}
end

local function transInfo(sendTo, localM, port, ...)
	if mTable and mTable[localM] then
		mTable[localM].send(sendTo, port, ...)
	elseif tTable then
		tTable[localM].send(...)
	end
end

local handler = {}
handler.CloseConnection = function(receiveM, sendM, port, connectDex)
	if connections[connectDex]["nextHop"] == 1 then
		connections[connectDex] = nil
		return
	else
		if string.find(connectDex, ":") then
			transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "CloseConnection", connections[connectDex]["origin"].."|"..connections[connectDex]["dest"].."|"..connections[connectDex]["ID"])
		end
		transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "CloseConnection", connectDex)
		connections[connectDex] = nil
	end
end

handler.Data = function (_, _, _, data, connectDex, order, origin)
	if string.find(connectDex, ":") and GERTe and string.sub(connectDex, 1, string.find(connectDex, ":")) ~= gAddress then
		local pipeDex = string.find(connectDex, "|")
		GERTe.transmitTo(string.sub(connectDex, 1, pipeDex), string.sub(connectDex, pipeDex+1, string.find(connectDex, "|", pipeDex)), data)
	elseif string.find(connectDex, ":") then
		transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "Data", data, connections[connectDex]["origin"].."|"..connections[connectDex]["dest"].."|"..connections[connectDex]["ID"], order)
	elseif connections[connectDex] then
		transInfo(connections[connectDex]["nextHop"], connections[connectDex]["sendM"], connections[connectDex]["port"], "Data", data, connectDex, order)
	end
end

handler.NewNode = function (receiveM, sendM, port, gAddres, nTier)
	if not gAddress then
		transInfo(sendM, receiveM, port, "NewNode", 0.0, 0)
	end
end

local function sendOKResponse(bHop, receiveM, recPort, dest, origin, ID)
	if dest==iAdd then
		storeConnection(origin, ID, dest)
		computer.pushSignal("GERTConnectionID", origin, ID)
	end
	if origin ~= iAdd then
		transInfo(bHop, receiveM, recPort, "RouteOpen", dest, origin, tostring(ID))
	end
end
--marked for fusion/rearrangement in v1.5
handler.OpenRoute = function (receiveM, sendM, port, dest, _, origin, ID)
	dest = tostring(dest)
	if (dest == "0.0") or (string.find(dest, ":") and string.sub(dest, 1, string.find(dest, ":")-1)~= tostring(gAddress) and GERTe) then -- If a GERTc address is entered, the GERTe component points to another endpoint, and GERTe is enabled, open the connection. Also applies to connections to the MNC.
		return sendOKResponse(sendM, receiveM, port, dest, origin, ID)
	elseif string.find(dest, ":") and string.sub(dest, 1, string.find(dest, ":")-1)== tostring(gAddress) then -- If a GERTc address is entered and the GERTe component points to the MNC, strip out the GERTe component and route it like normal
		dest = string.sub(dest, string.find(dest, ":")+1)
	end
	dest = tonumber(dest)
	if nodes[dest][0.0] then
		transInfo(nodes[dest]["add"], nodes[dest]["receiveM"], nodes[dest]["port"], "OpenRoute", dest, nil, origin, tostring(ID))
	elseif nodes[dest] then
	end
	local  doLoop = true
	local intermediary =""
	while doLoop do
		intermediary = accumulateIntermediary(dest)
	end
	--Insert code for sending on the connection open request
end

handler.RegisterNode = function (receiveM, sendM, port, originatorAddress, childTier, childTable)
	childTable = serialize.unserialize(childTable)
	childGA = storeChild(originatorAddress, receiveM, port, childTier) -- Store the node in the nodes table and retrieve the GERTi address of the node for sending it back.
	transInfo(sendM, receiveM, port, "RegisterComplete", originatorAddress, childGA)
	for key, value in pairs(childTable) do -- Load the node's neighbors into the nodes table
		nodes[childGA]["neighbors"][key] = math.floor(value["tier"])
		if key ~= 0 then -- Do not attempt to access the MNC in the nodes table
			nodes[key]["neighbors"][childGA]= math.floor(value["tier"])
		end
	end
end

handler.RemoveNeighbor = function (_, _, _, origin) -- Checks to see if the nodes table contains the appropriate client, and removes it if so. This also removes the node from other nodes' neighbor tables
	if nodes[origin] ~= nil then
		for key, value in pairs(nodes[origin]["neighbors"]) do
			nodes[key]["neighbors"][origin] = nil
		end
		nodes[origin] = nil
	end
end

local function receivePacket(_, receiveM, sendM, port, distance, code, ...)
	print(code)
	if handler[code] ~= nil then
		handler[code](receiveM, sendM, port, ...)
	end
end

-- GERTe Integration
local function readGMessage()
	-- keep calling GERTe.parse until it returns nil
	local errC, message = pcall(GERTe.parse)
	while not errC and message do
		local target = string.sub(message["target"], string.find(message["target"], ":")+1)
		if connections[message["source"]..target..0] ~= nil then
			handler["Data"](_, _, _, message["data"], target, message["source"], 0)
		else
			handler["OpenRoute"](nodes[target]["receiveM"], nodes[target]["port"], target, _, message["source"], 0)
			handler["Data"](_, _, _, message["data"], target, message["source"], 0)
		end
		errC, message = pcall(GERTe.parse)
	end
	if errC ~= true then
		print("GERTe has closed the connection")
		event.cancel(timerID)
		GERTe = nil
	end
end

local function safedown()
	if GERTe then
		GERTe.shutdown()
	end
	for key, value in pairs(timerID) do
		event.cancel(value)
	end
	filesystem.remove("/etc/networkTables.gert") -- remove the network tables file on proper shut down to allow for recovering the MNC from a corrupted network configuration.
end

local function loadAddress() -- load GERTi address file to restore cached GERTi addresses
	if filesystem.exists(directory) then
		print("Address file located; loading now.")
		local f = io.open(directory, "r")
		local newGAddress = f:read("*l")
		local newRAddress = f:read("*l")
		local highest = 0
		while newGAddress ~= nil do
			newGAddress = tonumber(newGAddress)
			savedAddresses[newRAddress]=newGAddress
			highest = newGAddress
			newGAddress = f:read("*l")
			newRAddress = f:read("*l")
		end
		f:close()
		highest = tostring(highest)
		local dividerDex = string.find(highest, "%.")
		addressP1 = math.floor(tonumber(string.sub(highest, 1, dividerDex-1)))
		addressP2 = math.floor(tonumber(string.sub(highest, dividerDex+1)+1))
	end
end

local function loadTables() -- reload the network tables if they have been cached after an improper restart
	if filesystem.exists("/etc/networkTables.gert") then
		print("Reloading cached network tables")
		local f = io.open("/etc/networkTables.gert", r)
		nodes = serialize.unserialize(f:read("*l"))
		connections = serialize.unserialize(f:read("*l"))
		f:close()
	end
end

-- Function that runs on a timer to cache the nodes and connections table. In this manner, they can be recovered successfully if the MNC is abruptly powered off
local function cacheNetworkTables()
	local f = io.open("/etc/networkTables.gert", "w")
	f:write(serialize.serialize(nodes).."\n")
	f:write(serialize.serialize(connections))
	f:close()
end

function realStart()
------------------------ Startup procedure
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
	
	if filesystem.exists("/lib/GERTeAPI.lua") then
		GERTe = require("GERTeAPI")
	end
	event.listen("modem_message", receivePacket)

	if GERTe then
		local file = io.open("/etc/GERTconfig.cfg", "r")
		gAddress = file:read()
		gKey = file:read()
		GERTe.startup()
		local success, errCode = GERTe.register(gAddress, gKey)
		if not success then
			io.stderr:write("Error in connecting to GEDS! Error code: "..errCode)
		end
		table.insert(timerID, event.timer(0.1, readGMessage, math.huge))
	end
	loadAddress()
	loadTables()
	table.insert(timerID, event.timer(30, cacheNetworkTables, math.huge))
	event.listen("shutdown", safedown)
	print("Setup Complete!")
end

function start()
	event.timer(5, realStart, 1)
end

function stop()
	event.ignore("modem_message", receivePacket)
	safedown()
end