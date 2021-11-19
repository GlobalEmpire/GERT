local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local shell = require("shell")

local args, opts = shell.parse(...)
local updatePort = 941
local updateAddress = 0.0
local ModuleFolder = "/usr/lib/"
local config = {}
local configPath = "/usr/lib/GERTUpdater.cfg"
local storedPaths = {}
local GERTUpdaterAPI = {}

GERTUpdaterAPI.GetLocalVersion = function(path)
    local versionHeader = ""
    local localCacheExists = fs.exists(path)
    if localCacheExists then
        local file = io.open(path, "r")
        versionHeader = file:read("*l")
        file:close()
    end
    return versionHeader
end

GERTUpdaterAPI.GetRemoteVersion = function(moduleName,socket)
    local size, state, version = 0, 0, ""
    local hadSocket = true
    if not socket then
        hadSocket = false
        socket = GERTi.openSocket(updateAddress,updatePort)
        local connectionComplete = event.pull(10, "GERTConnectionID", updateAddress, updatePort)
        if not connectionComplete then
            return false, 1 -- 1 means No Response From Address
        end
    end
    socket:write("ModuleUpdate",moduleName)
    local response = event.pull(10, "GERTData", updateAddress, updatePort)
    if not response then
        return false, 2 -- 2 means timeout
    end
    local data = socket:read()
    if type(data[1]) == "table" then 
        if data[1][1] == "U.RequestReceived" then
            response = event.pull(10, "GERTData", updateAddress, updatePort)
            if not response then
                return false, 2 -- 2 means timeout
            else
                data = socket:read()
                if type(data[1]) == "table" and  then
                    if data[1][1] == true then
                        size, state, version = data[1][2],data[1][3],data[1][4]
                        return true, size, state, version
                    else
                else
                    return false, 3, data[1] -- 3 means unknown error, passing back unexpected output
                end
            end
        else
            socket:close()
            return false, 3, data[1][1] -- 3 means unknown error, passing back unexpected output
        end
    else
        socket:close()
        return false, 3, data[1] -- 3 means unknown error, passing back unexpected output
    end
    if not hadSocket then
        socket:close()
    end
end





GERTUpdaterAPI.CheckForUpdate = function (moduleName)
    moduleName = moduleName or storedPaths
    local infoTable = {}
    local socket = GERTi.openSocket(updateAddress,updatePort)
    local connectionComplete = event.pull(10, "GERTConnectionID", updateAddress, updatePort)
    if not connectionComplete then 
        return false, 1 -- 1 means No Response From Address
    end
    if type(moduleName) == "table" then
        for trueModuleName, modulePath in moduleName do
            local localVersion = GERTUpdaterAPI.GetLocalVersion(modulePath)
            local localSize = fs.size(modulePath)
            local success, remoteSize, statusCode, remoteVersion = GERTUpdaterAPI.GetRemoteVersion()
            if success then
                
            else
                remoteVersion,remoteSize
            end
            infoTable[trueModuleName] = {localVersion,localSize,remoteVersion,remoteSize}
        end
    else

    end




end