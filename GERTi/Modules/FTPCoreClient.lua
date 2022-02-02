-- FTP Core - Separation Update (Release 1 Beta 1) |R1B1
local fs = require("filesystem")


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
FTPCore.GetLocalVersion = function(path)
    if path == nil then
        return false, INVALIDARGUMENT
    end
    local versionHeader = ""
    local localCacheExists = fs.exists(path) and (not fs.isDirectory(path))
    if localCacheExists then
        local file = io.open(path, "r")
        versionHeader = file:read("*l")
        file:close()
    else
        return false, NOLOCALFILE
    end
    return versionHeader
end