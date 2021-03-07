-- GERT v1.4 Build 3
local component = require("component")
local computer = require("computer")
local event = require("event")
local filesystem = require("filesystem")
local GERTe = nil
local os = require("os")
local serialize = require("serialization")
local mTable
local tTable

local nodes = {}
local connections = {}
local addressP1 = 0
local addressP2 = 1
local gAddress = nil
local gKey = nil
local timerID = nil
local savedAddresses = {}
local directory = "/etc/GERTaddresses.gert"
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

local function storeChild(rAddress, receiveM, port, tier)
	-- parents means the direct connections a computer can make to another computer that is a higher tier than it
	-- children means the direct connections a computer can make to another computer that is a lower tier than it
	local childGA
	if savedAddresses[rAddress] then
		childGA = savedAddresses[rAddress]
	end
	if not childGA then
		childGA = addressP1.."."..addressP2
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
	end
	childGA = tonumber(childGA)
	nodes[childGA] = {}
	nodes[childGA]["add"] = rAddress
	nodes[childGA]["receiveM"] = receiveM
	nodes[childGA]["tier"] = tonumber(tier)
	nodes[childGA]["port"] = tonumber(port)
	return childGA
end

local function storeConnection(origin, ID, dest, nextHop, sendM, port, lieAdd)
	local connectDex
	ID = math.floor(ID)
	if lieAdd then
		connectDex = origin.."|"..lieAdd.."|"..ID
	else
		connectDex = origin.."|"..dest.."|"..ID
	end
	connections[connectDex] = {}
	connections[connectDex]["origin"]=origin
	connections[connectDex]["dest"]=dest
	connections[connectDex]["ID"]=ID
	connections[connectDex]["nextHop"]=nextHop
	connections[connectDex]["sendM"] = sendM
	connections[connectDex]["port"]=port
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
-- Used in handler["OPENROUTE"]
local function routeOpener(dest, origin, receiveM, sendM, bHop, nextHop, hop2, recPort, transPort, ID, lieAdd)
	print("Opening Route")
    local function sendOKResponse()
		transInfo(bHop, sendM, recPort, "RouteOpen", (lieAdd or dest), origin, ID)
		storeConnection(origin, ID, dest, nextHop, receiveM, transPort, lieAdd)
	end
	
	if lieAdd or (not string.find(tostring(dest), ":")) then
		transInfo(nextHop, receiveM, transPort, "OpenRoute", dest, hop2, origin, ID)
		addTempHandler(3, "RouteOpen", function (eventName, recv, sender, port, distance, code, pktDest, pktOrig, ID)
			if (dest == pktDest) and (origin == pktOrig) then
				sendOKResponse()
				return true
			end
		end, function () end)
	elseif GERTe then
    	sendOKResponse()
	end
end

handler.OpenRoute = function (receiveM, sendM, port, dest, intermediary, origin, ID)
	local lieAdd
	dest = tostring(dest)
	if string.find(dest, ":") and string.sub(dest, 1, string.find(dest, ":")-1)~= tostring(gAddress) then
		return routeOpener(tonumber(dest), origin, receiveM, sendM, receiveM, receiveM, port, port, ID)
	elseif string.find(dest, ":") and string.sub(dest, 1, string.find(dest, ":")-1)== tostring(gAddress) then
		lieAdd = dest
		dest = string.sub(dest, string.find(dest, ":")+1)
	end
	dest = tonumber(dest)
	if nodes[dest][0.0] then
		return routeOpener(dest, origin, nodes[dest]["receiveM"], receiveM, sendM, nodes[dest]["add"], nodes[dest]["add"], port, nodes[dest]["port"], ID, lieAdd)
	end
	
	local interTier = 1000
	local nodeDex = dest
	local inter = ""
	while interTier > 1 do
		for i=1, nodes[nodeDex]["tier"] do
			if nodes[nodeDex][i] then
				for key,value in pairs(nodes[nodeDex][i]) do
					inter = value.."|"..inter
					interTier = i
					nodeDex = value
					break
				end
				break
			end
		end
	end
	local nextHop = tonumber(string.sub(inter, 1, string.find(inter, "|")-1))
	inter = string.sub(inter, string.find(inter, "|")+1)
	return routeOpener(dest, origin, nodes[nextHop]["receiveM"], receiveM, sendM, nodes[nextHop]["add"], inter, port, nodes[nextHop]["port"], ID, lieAdd)
end

handler.RegisterNode = function (receiveM, sendM, port, originatorAddress, childTier, childTable)
	childTable = serialize.unserialize(childTable)
	childGA = storeChild(originatorAddress, receiveM, port, childTier)
	transInfo(sendM, receiveM, port, "RegisterComplete", originatorAddress, childGA)
	for key, value in pairs(childTable) do
		if not nodes[childGA][value["tier"]] then
			nodes[childGA][value["tier"]] = {}
		end
		nodes[childGA][value["tier"]][key] = key
	end
end

handler.RemoveNeighbor = function (_, _, _, origination)
	if nodes[origination] ~= nil then
		nodes[origination] = nil
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
		event.cancel(timerID)
		GERTe.shutdown()
	end
end

function loadAddress()
	if filesystem.exists(directory) then
		print("Address file located; loading now.")
		local f = io.open(directory, "r")
		local newGAddress = f:read("*l")
		local newRAddress = f:read("*l")
		local highest = 0
		while newGAddress ~= nil do
			newGAddress = tonumber(newGAddress)
			savedAddresses[newRAddress]=newGAddress
			if newGAddress >= highest then
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
		timerID = event.timer(0.1, readGMessage, math.huge)
		event.listen("shutdown", safedown)
	end
	loadAddress()
	print("Setup Complete!")
end

function start()
	event.timer(5, realStart, 1)
end

function stop()
	event.ignore("modem_message", receivePacket)
	safedown()
end