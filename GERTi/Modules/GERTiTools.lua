-- GERTiTools Version 1|V1
local GERTi = require("GERTiClient")
local event = require("event")

local NOSOCKET = -1


local GERTiTools = {}


GERTiTools.SocketWithTimeout = function (Details,Timeout) -- Timeout defaults to 5 seconds. Details must be a keyed array with the address under "address" and the port/CID under "port"
    local socket = GERTi.openSocket(Details.address,Details.port)
    local serverPresence = false
    if socket then serverPresence = event.pullFiltered(Timeout or 5,function (eventName,oAdd,CID) return eventName=="GERTConnectionID" and oAdd==Details.address and CID==Details.port end) end
    if not serverPresence then
        socket:close()
        return false, NOSOCKET
    end
    return true, socket
end

return GERTiTools