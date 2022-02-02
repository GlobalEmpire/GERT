-- FTP Core - Separation Update (Release 1 Beta 1) |R1B1
local fs = require("filesystem")
local GERTi = require("GERTiClient")
local event = require("event")

--Error Codes
local INVALIDARGUMENT = 0
local NOSOCKET = -1
local TIMEOUT = -2
local NOSPACE = -3
local INTERRUPTED = -4
local NOLOCALFILE = -5
local UNKNOWN = -6
local SERVERRESPONSEFALSE = -7
local MODULENOTCONFIGURED = -8
local CANNOTCHANGE = -9
local UPTODATE = -10
local NOREMOTERESPONSE = -11

--Op Codes
local ALLGOOD = 0
local DOWNLOADED = 1
local ALREADYINSTALLED = 10


local FTPCore = {}

local function CreateValidSocket (address,port)
    local socket = GERTi.openSocket(address,port)
    local serverPresence = false
    if socket then serverPresence = event.pullFiltered(5,function (eventName,oAdd,CID) return eventName=="GERTConnectionID" and oAdd==address and CID==port end) end
    if not serverPresence then
        socket:close()
        return false, NOSOCKET
    end
    return socket
end

FTPCore.CheckData = function (file,address,port,user,auth)
    local socket,code = CreateValidSocket(address,port)
    if not socket then
        return false, code
    end
    socket:write("FTPGETFILEDATA",file,user,auth)
    local response = event.pullFiltered(5,function (eventName) return eventName=="modem_message" and #socket:read("-k")~=0 end)
    if not response then
        socket:close()
        return false, TIMEOUT
    end
    local data = socket:read()
    socket:close()
    if data[1] == file then
        return data -- In sequence: {File name as passed as parameter | File Size}
    else
        return false, UNKNOWN
    end
end

FTPCore.DownloadFile = function (file,destination,address,port,user,auth) --Provide file as relative path on server, destination as where it goes, address of the server.
    local remoteData = FTPCore.CheckData(file,address,port,user,auth)
    local remoteSize = remoteData[2]
    file = fs.name(file)
    local storageDrive = fs.get(destination .. "/" .. file)
    local remainingSpace = (storageDrive.spaceTotal()-storageDrive.spaceUsed())
    if remoteSize > remainingSpace - 1000 then
        return false, NOSPACE -- insufficient space for download
    end
    local socket = GERTi.openSocket(updateAddress,updatePort)
    local serverPresence = event.pullFiltered(5,function (event,oAdd,CID) return event=="GERTConnectionID" and oAdd==updateAddress and CID==updatePort end)
    if not serverPresence then
        socket:close()
        return false, NOSOCKET
    end
    local file = io.open(cacheFolder .. moduleName, "wb")
    local loop = false
    socket:write("RequestCache",moduleName)
    repeat
        local response = event.pullFiltered(5, DownloadFilter)
        if not response then
            socket:close()
            return false, TIMEOUT
        elseif response == "GERTConnectionClose" then
            if #socket:read("-k") > 1 then
                file:write(socket:read()[1])
            end
            loop = true
        else
            file:write(socket:read()[1])
            socket:write("ReadyForContinue")
        end
    until loop
    socket:close()
    file:close()
    if fs.size(cacheFolder .. moduleName) == remoteSize then
        return true
    else
        return false, INTERRUPTED -- connection interrupted
    end
end
