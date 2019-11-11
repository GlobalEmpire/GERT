-- GERT v1.1 Release
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local GERTe = nil
local os = require("os")
local serialize = require("serialization")
local modem = nil
local tunnel = nil

local nodes = {}
local connections = {}
local paths = {}

local addressP1 = 0
local addressP2 = 1
local gAddress = nil
local gKey = nil
local timerID = nil
local savedAddresses = {}

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

if filesystem.exists("/lib/GERTeAPI.lua") then
	GERTe = require("GERTeAPI")
end

local directory = os.getenv("_")
directory = filesystem.path(directory)
-- functions to store the children and then sort the table

-- this function adds a handler for a set time in seconds, or until that handler returns a truthful value (whichever comes first)
local function addTempHandler(timeout, code, cb, cbf)
	local function cbi(...)
        local evn, rc, sd, pt, dt, code2 = ...
        if code ~= code2 then return end
        if cb(...) then
        	return false
        end
	end
        event.listen("modem_message", cbi)
        event.timer(timeout, function ()
                event.ignore("modem_message", cbi)
                cbf()
        end)
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

local function storeChild(rAddress, port, tier)
	-- parents means the direct connections a computer can make to another computer that is a higher tier than it
	-- children means the direct connections a computer can make to another computer that is a lower tier than it
	local childGA
	if savedAddresses[rAddress] then
		childGA = savedAddresses[rAddress]
	end
	if not childGA then
		childGA = addressP1.."."..addressP2
		savedAddresses[rAddress] = childGA
		local f = io.open(directory.."GERTaddresses.gert", "a")
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
	end
	childGA = tonumber(childGA)
	nodes[childGA] = {}
	nodes[childGA]["add"] = rAddress
	nodes[childGA]["tier"] = tonumber(tier)
	nodes[childGA]["port"] = tonumber(port)
	nodes[childGA]["parents"] = {}
	nodes[childGA]["children"]={}
	return childGA
end

local function storeConnection(origin, ID, dest)
	if not connections[dest] then
		connections[dest] = {}
	end
	if not connections[dest][origin] then
		connections[dest][origin] = {}
	end
	connections[dest][origin][ID] = {}
end
local function storePath(origin, dest, nextHop, port)
if paths[origin] == nil then
		paths[origin] = {}
	end
	paths[origin][dest] = {}
	paths[origin][dest]["nextHop"] = nextHop
	paths[origin][dest]["port"] = port
end

local function transmitInformation(sendTo, port, ...)
	if (port ~= 0) and (modem) then
		return modem.send(sendTo, port, ...)
	elseif (tunnel) then
		return tunnel.send(...)
	end
end

local handler = {}
handler.CloseConnection = function(sendingModem, port, connectionID, destination, origin)
	if destination ~= iAddress then
		transmitInformation(paths[origin][destination]["nextHop"], paths[origin][destination]["port"], "CloseConnection", ID, destination, origin)
	end
	connections[destination][ID] = nil
end

handler.Data = function (sendingModem, port, data, dest, origin, ID, order)
	if string.find(dest, ":") and GERTe then
		if string.sub(dest, 1, string.find(dest, ":")) == gAddress then
			dest = string.sub(dest, 1, string.find(dest, ":"))
			transmitInformation(paths[origin][dest]["nextHop"], paths[origin][dest]["port"], "Data", data, dest, origin, ID, order)
		else
			GERTe.transmitTo(dest, origin, data)
		end
	elseif connections[dest] and connections[dest][origin] and connections[dest][origin][ID] then
		transmitInformation(paths[origin][dest]["nextHop"], paths[origin][dest]["port"], "Data", data, dest, origin, ID, order)
	end
end

handler.NewNode = function (sendingModem, port)
	transmitInformation(sendingModem, port, "RETURNSTART", 0.0, 0)
end
-- Used in handler["OPENROUTE"]
local function routeOpener(dest, origin, bHop, nextHop, hop2, recPort, transPort, ID)
	print("Opening Route")
    local function sendOKResponse(isDestination)
		transmitInformation(bHop, recPort, "RouteOpen", dest, origin)
		if isDestination then
			storeConnection(origin, ID, dest)
			storePath(origin, dest, nextHop, transPort)
		else
			storePath(origin, dest, nextHop, transPort)
		end
	end
    if not string.find(dest, ":") then
		transmitInformation(nextHop, trans, "OpenRoute", dest, hopTwo, origin, ID)
		if event.pull(3, "modem_message", nil, nil, nil, nil, "RouteOpen", nil, dest, origin) then
			sendOKResponse(false)
		end
	elseif GERTe then
    	sendOKResponse(true)
	end
end

handler.OpenRoute = function (sendingModem, port, dest, intermediary, origin, ID)
	if string.find(dest, ":") then
		return routeOpener(dest, origin, sendingModem, modem.address, modem.address, port, port, ID)
	end
	
	if nodes[dest]["parents"][0.0] then
		return routeOpener(dest, origin, sendingModem, nodes[dest]["add"], nodes[dest]["add"], port, nodes[dest]["port"], ID)
	end
	
	for key, value in pairs(nodes[dest]["parents"]) do
		if nodes[key]["parents"][0.0] then
			return routeOpener(dest, origin, sendingModem, value["add"], dest, port, value["port"], ID)
		end
	end
	
	for key, value in pairs(nodes[dest]["parents"]) do
		for key2, value2 in pairs(nodes[key]["parents"]) do
			if nodes[key2]["parents"][0.0] then
				return routeOpener(dest, origin, sendingModem, value2["add"], value["add"], port, value2["port"], ID)
			end
		end
	end
end

handler.RegisterNode = function (sendingModem, port, originatorAddress, childTier, childTable)
	childTable = serialize.unserialize(childTable)
	childGA = storeChild(originatorAddress, port, childTier)
	transmitInformation(sendingModem, port, "RegisterComplete", originatorAddress, childGA)
	for key, value in pairs(childTable) do
		if value["tier"] >= childTier then
			nodes[childGA]["children"][key] = value
			if nodes[key]["tier"] > childTier then
				nodes[key]["parents"][childGA]={}
				nodes[key]["parents"][childGA]["add"] = nodes[childGA]["add"]
				nodes[key]["parents"][childGA]["port"] = nodes[childGA]["port"]
				nodes[key]["parents"][childGA]["tier"] = nodes[childGA]["tier"]
			else
				nodes[key]["children"][childGA]={}
				nodes[key]["children"][childGA]["add"] = nodes[childGA]["add"]
				nodes[key]["children"][childGA]["port"] = nodes[childGA]["port"]
				nodes[key]["children"][childGA]["tier"] = nodes[childGA]["tier"]
			end
		elseif key ~= 0.0 then
			nodes[childGA]["parents"][key] = value
			nodes[key]["children"][childGA]={}
			nodes[key]["children"][childGA]["add"] = nodes[childGA]["add"]
			nodes[key]["children"][childGA]["port"] = nodes[childGA]["port"]
			nodes[key]["children"][childGA]["tier"] = nodes[childGA]["tier"]
		end
	end
end

handler.RemoveNeighbor = function (sendingModem, port, origination)
	if nodes[origination] ~= nil then
		nodes[origination] = nil
	end
end

local function receivePacket(eventName, receivingModem, sendingModem, port, distance, code, ...)
	print(code)
	if handler[code] ~= nil then
		handler[code](sendingModem, port, ...)
	end
end

-- GERTe Integration
local function readGMessage()
	-- keep calling GERTe.parse until it returns nil
	local errC, message = pcall(GERTe.parse)
	while not errC and message do
		local found = false
		local target = string.sub(message["target"], string.find(message["target"], ":")+1)
		if connections[target] ~= nil then
			found = true
			handler["Data"](_, _, message["data"], target, message["source"], 0)
		end
		if not found then
			handler["OpenRoute"](modem.address, 4378, target, _, message["source"], 0)
			storeConnection(message["source"], 0, target)
			handler["Data"](_, _, message["data"], target, message["source"], 0)
		end
		errC, message = pcall(GERTe.parse)
	end
	if errC then
		print("GERTe has closed the connection")
		event.cancel(timerID)
		GERTe = nil
	end
end

local function safedown()
	if GERTe then
		event.cancel(timerID)
		GERTe.shutdown()
	end
end
------------------------ Startup procedure
event.listen("modem_message", receivePacket)

if GERTe then
	local file = io.open(directory.."GERTconfig.cfg", "r")
	gAddress = file:read()
	gKey = file:read()
	GERTe.startup()
	local success, errCode = GERTe.register(gAddress, gKey)
	if not success then
		io.stderr:write("Error in connecting to GEDS! Error code: "..errCode)
	end
	timerID = event.timer(0.1, readGMessage, math.huge)
	event.listen("shutdown", safedown)
end

if filesystem.exists(directory.."GERTaddresses.gert") then
	print("Address file located; loading now.")
	local f = io.open(directory.."GERTaddresses.gert", "r")
	local newGAddress = f:read("*l")
	local newRAddress = f:read("*l")
	local highest = 0
	while newGAddress ~= nil do
		newGAddress = tonumber(newGAddress)
		savedAddresses[newRAddress]=newGAddress
		if newGAddress > highest then
			highest = newGAddress
		end
		newGAddress = f:read("*l")
		newRAddress = f:read("*l")
	end
	f:close()
	highest = tostring(highest)
	local dividerDex = string.find(highest, "%.")
	addressP1 = string.sub(highest, 1, dividerDex-1)
	addressP2 = string.sub(highest, dividerDex+1)+1
end
print("Setup Complete!")