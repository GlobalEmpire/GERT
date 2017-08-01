local comp = require "component"

if not comp then
	apiRequired("Component")
end

local fs = require "filesystem"

if not fs then
	apiRequired("Filesystem")
end

local proc = require "process"

if not proc then
	apiRequired("Process")
end

local api = {}
local card = comp.proxy(comp.list("internet")())
local socket
local path = fs.path(process.running())
local peers = {}
local connected

local version = "\1\0\0"
local readableVersion = "1.0.0"

if card.isTcpEnabled() == false then
	error("TCP Sockets are not permitted. If this is a server contact the administrator. If this is single player please modify the configuration file.")
end

function api.startup() --Will ALWAYS ensure gateway is connected
	if socket then
		return
	end
	local file = fs.open(path .. "peers.geds", "rb")
	while true do --Loading peers from file
		local ipNums = file:read(4)
		if ipNums == nil then break end
		local ip = formatIp(ipNums)
		local gatePort = tonumber(file:read(2))
		local peerPort = tonumber(file:read(2))
		table.insert(peers, {ip = ip, gate = gatePort, peer = peerPort})
	end
	fs.close(file)
	
	for num, peer in pairs(peer) do --Testing peers
		local temp = card.connect(peer.ip, peer.gate)
		local result, succ
		while true do
			result, succ = temp.finishConnect()
			if not result or succ then
				break
			end
		end
		if succ and result then
			temp.write(version) --Note to self: This seems broken. Check to make sure it's actually sub-byte
			if temp.read(3) ~= "\0\0\0" then
				socket = temp
				connected = peer
			end
			temp.close()
		end
	end

	error("Failed to find valid peer")
end

function api.register(addr, key)
	local cmd = "\1"
	local rawAddr = parseAddr(addr .. ".0.0")
	cmd = cmd .. rawAddr
	socket.write(cmd)
	local result = socket.read()
	if string.sub(result, 2, 2) == "\0" then
		return false, parseError(string.sub(result, 3, 3))
	end
	return true
end

function api.transmitTo(addr, data)
	if not socket then
		return false, "Not connected"
	end
	local cmd = "\2"
	local rawAddr = parseAddr(addr)
	cmd = cmd .. rawAddr
	if string.len(data) > 247 then
		return false, "Data too long"
	end
	cmd = cmd .. data
	socket.write(cmd)
	local result = socket.read()
	if string.sub(result, 2, 2) == "\0" then
		return false, parseError(string.sub(result, 3, 3))
	end
	return true
end

function api.shutdown()
	if socket then
		socket.write("\4")
		socket.close()
	end
end

local function apiRequired(name)
	error(name .. " API from OpenComputers required. Either this isn't an OpenComputer's computer, or this API is hidden")
end

local function formatIp(nums)
	ip = tostring(string.sub(nums, 1, 1)) .. "."
	ip = ip .. tostring(string.sub(nums, 2, 2)) .. "."
	ip = ip .. tostring(string.sub(nums, 3, 3)) .. "."
	ip = ip .. tostring(string.sub(nums, 4, 4))
	return ip
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
		return "NO_ROUTE"
	end
end

local function parseAddr(addr)
	local addrPart = "%d?%d?%d?%d"
	local addrSegment = addrPart .. "%." .. addrPart
	local externalUpper = string.match(addr, "(" .. addrPart .. ")%." .. addrPart .. "%." .. addrSegment)
	local externalLower = string.match(addr, addrPart .. "%.(" .. addrPart .. ")%." .. addrSegment)
	local internalUpper = string.match(addr, addrSegment .. "%.(" .. addrPart .. ")%." .. addrPart)
	local internalLower = string.match(addr, addrSegment .. "%." .. addrPart .. "%.(" .. addrPart .. ")")
	externalUpper = tonumber(externalUpper)
	externalLower = tonumber(externalLower)
	local chars = {
		string.char(externalUpper >> 4),
		string.char((externalUpper << 8 & 0xF0) | (externalLower >> 8)),
		string.char((externalLower << 8)),
		string.char(internalUpper >> 4),
		string.char((internalUpper << 8 & 0xF0) | (internalLower >> 8)),
		string.char((internalLower << 8)),
	}
	return chars:concat("")
end

local function unparseAddr(addr) 
	local chars = {
		addr:sub(1, 1):byte(),
		addr:sub(2, 2):byte(),
		addr:sub(3, 3):byte(),
		addr:sub(4, 4):byte(),
		addr:sub(5, 5):byte(),
		addr:sub(6, 6):byte()
	}
	parts = {
		tostring((chars[1] << 4) | (chars[2] >> 4)),
		tostring(((chars[2] & 0x0F) << 8) | chars[3]),
		tostring((chars[4] << 4) | (chars[5] >> 4)),
		tostring(((chars[5] & 0x0F) << 8) | chars[6]),
	}
	return parts:concat(".")
end

function api.parse()
	local msg = socket.read()
	local cmd = msg:sub(1, 1)
	msg:sub(2)

	if cmd == 2 then
		local addrPart = "%d?%d?%d?%d"
		local addrSegment = addrPart .. "%." .. addrPart
		local addr = unparseAddr(msg)
		local source = unpauseAddr(msg:sub(5))
		return {
			target = addr:match(addrSegment .. "%.(" .. addrSegment .. ")")
			source = source
			data = msg:sub(7)
		}
	elseif cmd == 4 then
		socket.close()
		error("Server closed connection")
	end
end

return api
