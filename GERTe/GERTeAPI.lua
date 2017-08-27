local comp = require "component"

if not comp then
	apiRequired("Component")
end

local api = {}
local card = comp.proxy(comp.list("internet")())
local socket
local connected

local version = "\1\0\0"
local readableVersion = "1.0.0"

if card.isTcpEnabled() == false then
	error("TCP Sockets are not permitted. If this is a server contact the administrator. If this is single player please modify the configuration file.")
end

local function apiRequired(name)
	error(name .. " API from OpenComputers required. Either this isn't an OpenComputer's computer, or this API is hidden")
end

local function formatIp(nums)
	return nums:byte(1) .. "." .. nums:byte(2) .. "." .. nums:byte(3) .. "." .. nums:byte(4)
end

local function parseError(err)
	err = string.byte(err)
	if err == 0 then
		return "VERSION"
	elseif err == 1 then
		return "BAD_KEY"
	elseif err == 2 then
		return "ALREADY_REGISTERED"
	elseif err == 3 then
		return "NOT_REGISTERED"
	elseif err == 4 then
		return "NO_ROUTE"
	end
end

local function parseAddr(addr)
	local addrPart = "%d?%d?%d?%d"
	local addrSegment = addrPart .. "%." .. addrPart
	local externalUpper = string.match(addr, "(" .. addrPart .. ")%." .. addrPart .. ":" .. addrSegment)
	local externalLower = string.match(addr, addrPart .. "%.(" .. addrPart .. "):" .. addrSegment)
	local internalUpper = string.match(addr, addrSegment .. ":(" .. addrPart .. ")%." .. addrPart)
	local internalLower = string.match(addr, addrSegment .. ":" .. addrPart .. "%.(" .. addrPart .. ")")
	externalUpper = tonumber(externalUpper)
	externalLower = tonumber(externalLower)
	internalUpper = tonumber(internalUpper)
	internalLower = tonumber(internalLower)
	local chars = {
		string.char(externalUpper >> 4),
		string.char(((externalUpper & 0x0F) << 4) | (externalLower >> 8)),
		string.char(externalLower & 0xFF),
		string.char(internalUpper >> 4),
		string.char(((internalUpper & 0x0F) << 4) | (internalLower >> 8)),
		string.char(internalLower & 0xFF),
	}
	return table.concat(chars, "")
end

local function unparseAddr(addr) 
	local chars = {
		addr:byte(1),
		addr:byte(2),
		addr:byte(3),
		addr:byte(4),
		addr:byte(5),
		addr:byte(6)
	}
	parts = {
		tostring((chars[1] << 4) | (chars[2] >> 4)),
		tostring(((chars[2] & 0x0F) << 8) | chars[3]),
		tostring((chars[4] << 4) | (chars[5] >> 4)),
		tostring(((chars[5] & 0x0F) << 8) | chars[6]),
	}
	return parts[1] .. "." .. parts[2] .. ":" .. parts[3] .. "." .. parts[4]
end

function api.parse()
	local cmd = socket.read(1) or ""
	if cmd == "" then
		return nil
	end

	if cmd == 2 then
		local addrPart = "%d?%d?%d?%d"
		local addrSegment = addrPart .. "%." .. addrPart
		local addrs = socket.read(12)
		local addr = unparseAddr(msg)
		local source = unpauseAddr(msg:sub(7))
		local length = socket.read(1)
		return {
			target = addr:match(addrSegment .. "%.(" .. addrSegment .. ")"),
			source = source,
			data = socket.read(length)
		}
	elseif cmd == 4 then
		socket.close()
		error("Server closed connection")
	end
end

function api.startup() --Will ALWAYS ensure gateway is connected
	if socket then
		return
	end
	local file = io.open("peers.geds", "rb")
	local peers = {}
	while true do --Loading peers from file
		local ipNums = file:read(4)
		if ipNums == nil then break end
		local ip = formatIp(ipNums)
		local gatePort = (file:read(1):byte() << 8) | file:read(1):byte()
		local peerPort = (file:read(1):byte() << 8) | file:read(1):byte()
		table.insert(peers, {ip = ip, gate = gatePort, peer = peerPort})
	end
	file:close()
	
	for num, peer in pairs(peers) do --Testing peers
		local temp = card.connect(peer.ip, peer.gate)
		local result, err
		while true do
			result, err = temp.finishConnect()
			if result or err then
				break
			end
		end
		if not err and result then
			temp.write(version)
			local response = ""
			while response:len() < 1 do
				response = temp.read(5) or ""
			end
			if response:sub(1, 3) == "\0\1" .. version:sub(1, 1) then
				socket = temp
				connected = peer
				return
			else
				temp.close()
			end
		end
	end

	error("Failed to find valid peer")
end

function api.register(addr, key)
	local cmd = "\1"
	local rawAddr = parseAddr(addr .. ":0.0")
	cmd = cmd .. rawAddr:sub(1, 3) .. key
	socket.write(cmd)
	local result = ""
	while result:len() < 1 do
		result = socket.read() or ""
	end
	if string.sub(result, 2, 2) == "\0" then
		return false, parseError(string.sub(result, 3, 3))
	end
	return true
end

function api.transmitTo(addr, from, data)
	if not socket then
		return false, "Not connected"
	end
	local cmd = "\2"
	local rawAddr = parseAddr(addr)
	local rawFrom = parseAddr("0.0:" .. from)
	rawFrom = rawFrom:sub(4)
	cmd = cmd .. rawAddr .. rawFrom
	if string.len(data) > 255 then
		return false, "Data too long"
	end
	cmd = cmd .. string.char(data:len()) .. data
	socket.write(cmd)
	local result = ""
	while result:len() < 1 do
		result = socket.read() or ""
	end
	if string.sub(result, 2, 2) == "\0" then
		return false, parseError(string.sub(result, 3, 3))
	end
	return true
end

function api.shutdown()
	if socket then
		socket.write("\4")
		socket.close()
		socket = nil
	end
end

return api
