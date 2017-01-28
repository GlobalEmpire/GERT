--Assuming Lua Socket availability

--For quick reference for communication purposes:
--Follows similar standard to HTTP
--GENS %vers%\r\n
--%dest% %origin%\r\n\r\n

--GENS %vers%\r\n
--FOUND: %ip%\r\n OR ERR: %err%\r\n

--Warning: Designed in Lua 5.1 for compatibilty with LuaForWindows (Developer's Version)

logFile = io.open("log.txt", "a")
logFile:write("[" .. os.time() .. "] -----     Initializing      -----\n")

function log(str)
	logFile:write("[" .. os.time() .. "] " .. str .. "\n")
	logFile:flush()
end

socket = require "socket"
ipSeg = "%d?%d?%d"
ipPat = ipSeg .. "%." .. ipSeg .. "%." .. ipSeg .. "%." .. ipSeg
gertPat = "%d%d%d%-%d%d%d%d"
fullPat = gertPat .. "%-%d%d%d%d"

vers = "0.0"
compatible = {}
compatible["0.0"] = true

serv = socket.tcp()
io.stdout:write("Run Server On Port: ")
port = io.read()
serv:bind("*", port)
serv:listen(10)
serv:settimeout(0) --Allows non-blocking calls
log("Server Open on " .. port)

function lookup(id)
	return database[string.match(id, "(" .. gertPat .. ")%-%d%d%d%d")] or nil
end

function send(data, sock)
	local str = "GENS " .. vers .. "\r\n"
	for k, line in pairs(data) do
		str = str .. line .. "\r\n"
	end
	sock:send(str .. "\r\n") --Optimized, works around Nagel's without disabling it
	sock:close()
end

function main()
	local client = serv:accept() --WARNING: Will block, this code won't close cleanly
	if not client then return end --If client isn't waiting execute other operations
	local header = client:receive() --TODO: Implement something like socket.select or something
	if string.sub(header, 1, 4) ~= "GENS" then
		client:send("HTTP/1.1 400 Bad Request\r\n") --Why not? It's universal
		send({"ERR: NOT GENS"}, client)
		log("[W] Non-GENS from " .. client:getpeername())
		return
	elseif not compatible[string.sub(header, 6)] then
		send({"ERR: NOT COMPATIBLE"}, client)
		log("[W] Non-compatible GENS from " .. client:getpeername())
		return
	end
	local request = client:receive()
	local reqID, origin = string.match(request, "(" .. fullPat .. ") (" .. fullPat .. ")")
	local tracker = origin .. " (" .. client:getpeername() ..")"
	local map = lookup(reqID)
	if not map then
		send({"ERR: NOT FOUND"}, client)
		log("[E] " .. tracker .. " attempted to find " .. reqID .. " but failed")
		return
	end
	send({"FOUND: " .. map}, client)
	log("[I] " .. tracker .. " found " .. reqID .. " (" .. map .. ")")
end

log("Building Database")

database = {}
file = io.open("database.txt", "r")

for entry in function() return file:read() end do
	local id, addr = string.match(entry, "(" .. gertPat .. ") = (" .. ipPat .. ")")
	database[id] = addr
end

log("-----Initialization Finished-----")

while true do --Primary execution loop
	local succ, result = pcall(main)
	if not succ then
		local line, err = string.match(result, ".+:(%d+): (.+)")
		if err == "interrupted!" then
			log("[I] User requested close")
			os.exit(0)
		end
		log("[C] Crashed with error: " .. err .. " on line " .. line)
		log("[C] Report crash please!")
		os.exit(1)
	end
end
