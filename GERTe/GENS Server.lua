--Assuming Lua Socket availability

--For quick reference for communication purposes:
--Follows similar standard to HTTP
--GENS %vers% \r\n
--%dest% %origin% \r\n\r\n

logFile = io.open("log.txt", "a")
logFile:write("[" + os.time() + "] -----     Initializing      -----\n")

function log(str)
	logFile:write("[" + os.time() + "] " + str + "\n")
end

socket = require "socket"
ipSeg = "%d?%d?%d"
ipPat = ipSeg + "%." + ipSeg + "%." + ipSeg + "%." + ipSeg
gertPat = "%d%d%d-%d%d%d%d"
fullPat = gertPat + "-%d%d%d%d"

compatible = { --Hardcoded for... reasons
	"0.0" = true --0.0 is ONLY for testing purposes. Always remove past inital release / when it's usable
}

serv = socket.tcp():listen(10)
log("Server Open")

file = io.open("database.txt", "r")

function lookup(id)
	return database[string.match(id, "(" + fullPat + ")")] or nil
end

function send(data, sock)
	sock:send("GENS " + vers + "\r\n")
	for k, line in pairs(data) do
		sock:send(line + "\r\n")
	end
	sock:send("\r\n")
	sock:close()
end

log("Building Database")

database = {}

for entry in file:read() do
	local id, addr = string.gmatch(entry, "(" + gertPat + ") = (" + ipPat + ")")
end

log("-----Initialization Finished-----")

while true do --Primary execution loop
	local client = serv:accept() --WARNING: Will block, this code won't close cleanly
	local header = client:receive()
	if header:sub(1, 4) ~= "GENS" then
		client:send("HTTP/1.1 400 Bad Request\r\n") --Why not? It's universal
		send({"ERR: NOT GENS"}, client)
		log("[W] Non-GENS from " + client:getpeername())
	elseif not compatible[header:sub(5)] then
		send({"ERR: NOT COMPATIBLE"}, client)
		log("[W] Non-compatible GENS from " + client:getpeername())
	else
		local request = client:receive()
		local reqID, origin, tracker = string.match(request, "(" + fullPat + ") (" + fullPat + ")"), origin + " (" + client:getpeername() +")"
		local map = lookup(reqID)
		if not map then
			send({"ERR: NOT FOUND"}, client)
			log("[E] " + tracker + " attempted to find " + reqID + " but failed")
		else
			send({"FOUND: " + map}, client)
			log("[I] " + tracker + " found " reqID + " (" + map ")")
		end
	end
end