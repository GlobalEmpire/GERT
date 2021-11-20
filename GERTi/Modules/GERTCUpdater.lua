local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local shell = require("shell")
local srl = require("serialization")
local SafeUpdater = require("SafeUpdater")

local args, opts = shell.parse(...)
local updatePort = 941
local updateAddress = 0.0
local moduleFolder = "/usr/lib/"
local cacheFolder = "/.moduleCache/"
local config = {}
local configPath = "/usr/lib/GERTUpdater.cfg"
local storedPaths = {}
local GERTUpdaterAPI = {}


local function eventBeep (freq,rep)
    computer.beep(freq,rep)
end

if opts.n then
    event.listen("UpdateAvailable", eventBeep(1000))
    event.listen("ReadyForInstall", eventBeep(1500))
    event.listen("InstallComplete", eventBeep(2000,2))

end

local function parseConfig ()
    local configFile = io.open(configPath, "r")
    config = srl.deserialize(configFile:read("*l"))
    local tempPath = configFile:read("*l")
    while tempPath ~= "" do
        storedPaths[fs.name(tempPath)] = tempPath
        tempPath = configFile:read("*l")
    end
    configFile:close()
    return config,storedPaths
end

local function writeConfig (config,storedPaths)
    local configFile = io.open(configPath,"w")
    configFile:write(srl.deserialize(config))
    for name,path in pairs(storedPaths) do 
        configFile:write("\n"..path)
    end
    configFile:close()
end


if not fs.exists(configPath) then
    config["AutoUpdate"] = false
    writeConfig(config,storedPaths)
else
    config,storedPaths = parseConfig()
end

if not fs.isDirectory(cacheFolder) then
    fs.makeDirectory(cacheFolder)
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

local function DownloadFilter (event, iAdd, dAdd, CID)
    if (iAdd == updateAddress or dAdd == updateAddress) and (dAdd == updatePort or CID == updatePort) then
        if event == "GERTConnectionClose" or event == "GERTData" then
            return true
        end
    end
    return false
end


local function DownloadModuleToCache (moduleName)
    local socket = GERTi.openSocket(updateAddress,updatePort)
    local connectionComplete = event.pull(10, "GERTConnectionID", updateAddress, updatePort)
    if not connectionComplete then 
        return false, 1 -- 1 means No Response From Address
    end
    local file = io.open(cacheFolder .. moduleName, "wb")
    local loop = true
    while loop do
        socket:write("RequestCache",moduleName)
        local response = event.pullFiltered(10, DownloadFilter)
        if not response then
            socket:close()
            return false, 2 -- 2 means timeout
        elseif response == "GERTConnectionClose" then
            loop = false
        else
            file:write(socket:read()[1])
        end
    end
    socket:close()
    file:close()
    return true
end


GERTUpdaterAPI.DownloadUpdate = function (moduleName,infoTable,InstallWhenReady)
    if not moduleName then 
        return false, 2, 1 -- 2 means Incorrect Argument, third parameter defines which
    end
    if InstallWhenReady == nil then 
        InstallWhenReady = config["AutoUpdate"]
    end
    if type(moduleName) == "string" then
        if not(type(infoTable) == "table" and type(infoTable[1]) == "string") then
            success, infoTable = GERTUpdaterAPI.CheckForUpdate(fs.name(moduleName))
            if not success then
                return false, 1 -- Could not establish connection to server
            end
        end
        if infoTable[1] ~= infoTable[3] and infoTable[4] ~= 0 then
            local success, code = DownloadModuleToCache(fs.name(moduleName))
            if success then
                SafeUpdater.register(storedPaths[moduleName], cacheFolder .. moduleName,InstallWhenReady) -- Queues program to be installed on next reboot
                event.push("UpdateAvailable",moduleName)
            else

            end
        else
            return false, 3 -- Already Up To Date
        end
    elseif type(moduleName) == "table" then

    else
        return false, 2 -- 2 means Incorrect Argument, third parameter defines which
    end

end

GERTUpdaterAPI.InstallUpdate = function (moduleName)
end

GERTUpdaterAPI.InstallNewModule = function(moduleName)
end

GERTUpdaterAPI.RemoveModule = function(moduleName)
end

GERTUpdaterAPI.AutoUpdate = function()
    return config["AutoUpdate"]
end

GERTUpdaterAPI.ChangeConfig = function(setting,newValue)
    config,storedPaths = parseConfig()
    config[setting] = newValue
    local configFile = io.open(configPath,"r")
    local _ = configFile:read("*l")
    local tempConfig = configFile:read("*a")
    configFile:close()
    local configFile = io.open(configPath,"w")
    configFile:write(srl.deserialize(config) .. "\n")
    configFile:write(tempConfig)
    configFile:close()
    return true
end

GERTUpdaterAPI.RunFullCheck = function() 
end