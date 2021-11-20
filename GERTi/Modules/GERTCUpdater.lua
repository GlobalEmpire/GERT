local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local shell = require("shell")
local srl = require("serialization")

local args, opts = shell.parse(...)
local updatePort = 941
local updateAddress = 0.0
local ModuleFolder = "/usr/lib/"
local config = {}
local configPath = "/usr/lib/GERTUpdater.cfg"
local storedPaths = {}
local GERTUpdaterAPI = {}

if not fs.exists(configPath) then
    local configFile = io.open(configPath,"w")
    configFile:write()
end





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
    local size, state, version = "", 0, ""
    local hadSocket = true
    if not socket then
        hadSocket = false
        socket = GERTi.openSocket(updateAddress,updatePort)
        local connectionComplete = event.pull(10, "GERTConnectionID", updateAddress, updatePort)
        if not connectionComplete then
            return false, 1 -- 1 means could not establish socket
        end
    end
    socket:write("ModuleUpdate",moduleName)
    local response = event.pull(10, "GERTData", updateAddress, updatePort)
    if not response then
        if not hadSocket then
            socket:close()
        end    
        return false, 2 -- 2 means timeout
    end
    local data = socket:read()
    if type(data[1]) == "table" then 
        if data[1][1] == "U.RequestReceived" then
            response = event.pull(10, "GERTData", updateAddress, updatePort)
            if not response then
                if not hadSocket then
                    socket:close()
                end            
                return false, 2 -- 2 means timeout
            else
                data = socket:read()
                if type(data[1]) == "table" and  then
                    if data[1][1] == true then
                        size, state, version = data[1][2], data[1][3] or 0, data[1][4] or ""
                        if not hadSocket then
                            socket:close()
                        end                    
                        return true, state, size, version
                    else
                        if not hadSocket then
                            socket:close()
                        end
                        return false, state, size, version
                    end
                else
                    if not hadSocket then
                        socket:close()
                    end                
                    return false, 3, data[1] -- 3 means unknown error, passing back unexpected output
                end
            end
        else
            if not hadSocket then
                socket:close()
            end
            return false, 3, data[1] -- 3 means unknown error, passing back unexpected output
        end
    else
        if not hadSocket then
            socket:close()
        end
        return false, 3, data[1] -- 3 means unknown error, passing back unexpected output
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
            local localVersion,localSize = GERTUpdaterAPI.GetLocalVersion(modulePath),fs.size(modulePath)
            local success, statusCode, remoteSize, remoteVersion = GERTUpdaterAPI.GetRemoteVersion(moduleName,socket)
            infoTable[trueModuleName] = {localVersion,localSize,remoteVersion,remoteSize,statusCode}
        end
    else
        local localVersion,localSize = GERTUpdaterAPI.GetLocalVersion(modulePath),fs.size(modulePath)
        local success, statusCode, remoteSize, remoteVersion = GERTUpdaterAPI.GetRemoteVersion(moduleName,socket)
        infoTable = {localVersion,localSize,remoteVersion,remoteSize,statusCode}
    end
    socket:close()
    return true, infoTable
end

GERTUpdaterAPI.DownloadUpdate = function (moduleName)

end

GERTUpdaterAPI.InstallUpdate = function (moduleName)

end