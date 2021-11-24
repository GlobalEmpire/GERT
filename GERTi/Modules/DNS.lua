-- DNS v1.5 Build 11
local event = require("event")
local filesystem = require("filesystem")
local MNCAPI
local DNSEntries = {}
local sockets = {}

local function checkConnection(_, origin, ID)
	if ID == 53 then
		local newSocket = MNCAPI.openSocket(origin, ID)
		sockets[origin] = newSocket
	end
end
local function checkData(_, origin, ID)
	if sockets[origin] and ID == 53 then
		local data = sockets[origin]:read()
		origin = tonumber(origin)
		if data[1][1] == "Register Name" then
			if not DNSEntries[data[1][2]] and (origin == data[1][3] or origin == 0.0)then
                DNSEntries[data[1][2]] = data[1][3]
            end
		elseif data[1][1] == "DNSResolve" then
			if DNSEntries[data[1][2]] then
				sockets[origin]:write(DNSEntries[data[1][2]])
			else
				sockets[origin]:write(false)
			end
		elseif data[1][1] == "Remove Name" then
			if not DNSEntries[data[1][2]] and (origin == data[1][3] or origin == 0.0)then
                DNSEntries[data[1][2]] = nil
            end
		end
	end
end
local function closeSockets(_, origin, dest, ID)
	if ID == 53 and dest == 0.0 then
		sockets[origin]:close()
		sockets[origin] = nil
	end
end

function start()
    if filesystem.exists("/lib/GERTiClient.lua") then
		while not package.loaded["GERTiClient"] do
			os.sleep(1)
		end
		os.sleep(1)
		MNCAPI = require("GERTiClient")
    else
        return
	end
    if MNCAPI.getEdition() ~= "MNCAPI" then
        return
    end
    event.listen("GERTConnectionID", checkConnection)
    event.listen("GERTData", checkData)
    event.listen("GERTConnectionClose", closeSockets)
	MNCAPI.registerNetworkService("DNS", 53)
	DNSEntries["MNC"] = 0.0
end