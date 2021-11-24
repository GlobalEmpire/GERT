-- GUS Core Component - Beta 1.1
local computer = require("computer")
local GERTi = require("GERTiClient")
local fs = require("filesystem")
local event = require("event")
local srl = require("serialization")
local shell = require("shell")
local rc = require("rc")
if fs.exists("/etc/rc.d/SafeUpdater.lua") and not rc.loaded.SafeUpdater then
    shell.execute("rc SafeUpdater enable")
end

local args, opts = shell.parse(...)
local updatePort = 941
local updateAddress = 0.0
local moduleFolder = "/lib/"
local cacheFolder = "/.moduleCache/"
local config = {}
local configPath = "/etc/GERTUpdater.cfg"
local GERTUpdaterAPI = {}

local function eventBeep (freq,rep)
    return function () computer.beep(freq,rep) end
end

if not opts.n then
    event.listen("UpdateAvailable", eventBeep(1000))
    event.listen("ReadyForInstall", eventBeep(1500))
    event.listen("InstallComplete", eventBeep(2000,0.5))
end

local function ParseConfig ()
    local configFile = io.open(configPath, "r")
    config = srl.unserialize(configFile:read("*l"))
    local tempName = configFile:read("*l")
    local tempPath = configFile:read("*l")
    local storedPaths = {}
    while tempPath ~= "" and tempPath do
        storedPaths[tempName] = tempPath
        tempName = configFile:read("*l")
        tempPath = configFile:read("*l")
    end
    configFile:close()
    return config,storedPaths
end

local function writeConfig (config,storedPaths)
    local configFile = io.open(configPath,"w")
    configFile:write(srl.serialize(config))
    for name,path in pairs(storedPaths) do 
        configFile:write("\n"..name)
        configFile:write("\n"..path)
    end
    configFile:close()
end

local function ParseSafeList ()
    if not fs.exists("/.SafeUpdateCache.srl") then 
        return {}
    end
    local file = io.open("/.SafeUpdateCache.srl", "r")
    local rawData = file:read("*a")
    file:close()
    local parsedData = srl.unserialize(rawData)
    return parsedData or {}
end

local function AddToSafeList (moduleName,currentPath,cachePath,installWhenReady)
    if not installWhenReady then
        installWhenReady = false
    else
        installWhenReady = true
    end
    local parsedData = ParseSafeList()
    parsedData[moduleName] = {currentPath,cachePath,installWhenReady}
    local file = io.open("/.SafeUpdateCache.srl", "w")
    file:write(srl.serialize(parsedData))
    file:close()
    return true
end

local function RemoveFromSafeList (moduleName)
    local parsedData = ParseSafeList()
    if not parsedData[moduleName] then
        return false
    end
    fs.remove(parsedData[moduleName][2])
    parsedData[moduleName] = nil
    local file = io.open("/.SafeUpdateCache.srl", "w")
    file:write(srl.serialize(parsedData))
    file:close()
    return true
end

if not fs.exists(configPath) then -- Creates the config file if it does not exist
    config["AutoUpdate"] = false
    writeConfig(config,storedPaths)
else
    config,storedPaths = ParseConfig()
end

if not fs.isDirectory(cacheFolder) then
    fs.makeDirectory(cacheFolder)
end

GERTUpdaterAPI.GetLocalVersion = function(path)
    local versionHeader = ""
    local localCacheExists = fs.exists(path) and (not fs.isDirectory(path))
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
    if not(moduleName) or type(moduleName) ~= "string" then
        return false, 0 -- 0 means moduleName not provided or invalid type
    end
    if not socket then
        hadSocket = false
        socket = GERTi.openSocket(updateAddress,updatePort)
        local serverPresence = false
        if socket then serverPresence = event.pullFiltered(5,function (event,oAdd,CID) return event=="GERTConnectionID" and oAdd==updateAddress and CID==updatePort end) end
        if not serverPresence then
            if not hadSocket then
                if socket then socket:close() end
            end
            return false, -1 -- -1 means could not establish socket
        end
    end
    socket:write("ModuleUpdate",moduleName)
    local response = event.pullFiltered(5,function (event) return event=="modem_message" and #socket:read("-k")~=0 end)
    if not response then
        if not hadSocket then
            socket:close()
        end
        return false, -2 -- -2 means timeout
    end
    local data = socket:read()
    if type(data[1]) == "table" then 
        if data[1][1] == "U.RequestReceived" then
            local response = event.pullFiltered(5,function (event) return event=="modem_message" and #socket:read("-k")~=0 end)
            if not response then
                if not hadSocket then
                    socket:close()
                end            
                return false, -2 -- -2 means timeout
            else
                data = socket:read()
                if type(data[1]) == "table" then
                    if data[1][1] == true then
                        size, state, version = data[1][2], data[1][3], data[1][4]
                        if not hadSocket then
                            socket:close()
                        end                    
                        return true, state, size, version
                    else
                        if not hadSocket then
                            socket:close()
                        end
                        return false, -4, data[1][2] -- -4 means Server Responded False
                    end
                else
                    if not hadSocket then
                        socket:close()
                    end                
                    return false, -3, data[1] -- -3 means unknown error, passing back unexpected output
                end
            end
        else
            if not hadSocket then
                socket:close()
            end
            return false, -3, data[1] -- -3 means unknown error, passing back unexpected output
        end
    else
        if not hadSocket then
            socket:close()
        end
        return false, -3, data[1] -- -3 means unknown error, passing back unexpected output
    end
end

GERTUpdaterAPI.CheckForUpdate = function (moduleName)
    local config, storedPaths = ParseConfig()
    moduleName = moduleName or storedPaths
    local infoTable = {}
    local socket = GERTi.openSocket(updateAddress,updatePort)
    local serverPresence = event.pullFiltered(5,function (event,oAdd,CID) return event=="GERTConnectionID" and oAdd==updateAddress and CID==updatePort end)
    if not serverPresence then
        if socket then socket:close() end
        return false, 1 -- 1 means could not establish socket
    end
    if type(moduleName) == "table" then
        for trueModuleName, modulePath in pairs(moduleName) do
            local localVersion,localSize = GERTUpdaterAPI.GetLocalVersion(modulePath),fs.size(modulePath)
            local success, statusCode, remoteSize, remoteVersion = GERTUpdaterAPI.GetRemoteVersion(moduleName,socket)
            infoTable[trueModuleName] = {localVersion,localSize,remoteVersion,remoteSize,statusCode,success}
        end
    else
        local modulePath = storedPaths[moduleName]
        local localVersion,localSize = GERTUpdaterAPI.GetLocalVersion(modulePath),fs.size(modulePath)
        local success, statusCode, remoteSize, remoteVersion = GERTUpdaterAPI.GetRemoteVersion(moduleName,socket)
        infoTable = {localVersion,localSize,remoteVersion,remoteSize,statusCode,success}
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


local function DownloadModuleToCache (moduleName,remoteSize)
    if not remoteSize then
        local a, b, d = 0,0,0
        a,b,remoteSize,d = GERTUpdaterAPI.GetRemoteVersion(moduleName)
        if not a then
            return a, b, remoteSize, d
        end
    end
    local storageDrive = fs.get(cacheFolder .. moduleName)
    local remainingSpace = (storageDrive.spaceTotal()-storageDrive.spaceUsed())
    if remoteSize > remainingSpace - 200 then
        return false, 3 -- 3 means insufficient space for update
    end
    local socket = GERTi.openSocket(updateAddress,updatePort)
    local serverPresence = event.pullFiltered(5,function (event,oAdd,CID) return event=="GERTConnectionID" and oAdd==updateAddress and CID==updatePort end)
    if not serverPresence then
        socket:close()
        return false, 1 -- 1 means could not establish socket
    end
    local file = io.open(cacheFolder .. moduleName, "wb")
    local loop = false
    repeat
        socket:write("RequestCache",moduleName)
        local response = event.pullFiltered(10, DownloadFilter)
        if not response then
            socket:close()
            return false, 2 -- 2 means timeout
        elseif response == "GERTConnectionClose" then
            loop = true
        else
            file:write(socket:read()[1])
        end
    until loop
    socket:close()
    file:close()
    if fs.size(file) == remoteSize then
        return true
    else
        return false, 4 -- 4 means connection interrupted
    end
end

GERTUpdaterAPI.Register = function (moduleName,currentPath,cachePath,installWhenReady)
    local error, success = pcall(AddToSafeList(moduleName,currentPath,cachePath,installWhenReady))
    if not(error and success) then
        return error, success
    end
    if installWhenReady then
        event.push("UpdateAvailable",moduleName)
    end
    return true
end

GERTUpdaterAPI.DownloadUpdate = function (moduleName,infoTable,InstallWhenReady) -- run with no arguments to do a check and cache download of all modules. If InstallWhenReady is true here or in the defaults then it will install the update when the event is received from the concerned program
    if not moduleName then 
        config, moduleName = ParseConfig()
    end
    if InstallWhenReady == nil then 
        InstallWhenReady = config["AutoUpdate"]
    end
    if type(moduleName) == "string" then
        if not(type(infoTable) == "table" and type(infoTable[1]) == "string") then
            print(moduleName)
            local success, infoTable = GERTUpdaterAPI.CheckForUpdate(moduleName)
            if not success then
                return success, 1, infoTable
            end
        end
        if infoTable[1] ~= infoTable[3] and infoTable[4] ~= 0 then
            local success, code = DownloadModuleToCache(moduleName,infoTable[4])
            if success then
                return GERTUpdaterAPI.Register(moduleName,storedPaths[moduleName], cacheFolder .. moduleName,InstallWhenReady),infoTable -- Queues program to be installed on next reboot
            else
                return success, code
            end
        else
            return false, -1 -- Already Up To Date
        end
    elseif type(moduleName) == "table" then
        local resultTable = {}
        local tempTable = {}
        local counter = false
        for name, path in pairs(moduleName) do
            local information = infoTable[name]
            if type(information) ~= "table" then
                tempTable[name] = path
                counter = true
            end
        end
        if counter then
            local success, tempLocalTable = GERTUpdaterAPI.CheckForUpdate(tempTable)
            if not success then
                return success,1, tempLocalTable -- 1 means could not validate infoTable information
            end
            for k,v in pairs(tempLocalTable) do
                infoTable[k] = v
            end
        end
        for name, path in pairs(moduleName) do
            local information = infoTable[name]
            if information[1] ~= information[3] and information[4] ~= 0 then
                local success, code = DownloadModuleToCache(name,information[4])
                if success then
                    resultTable[name] = table.pack(GERTUpdaterAPI.Register(name,storedPaths[name], cacheFolder .. name,InstallWhenReady))-- Queues program to be installed on next reboot
                    for k,v in ipairs(information) do
                        table.insert(resultTable[name],v)
                    end
                else
                    resultTable[name] = {success, code}
                end
            else
                resultTable[name] = {false, -1} -- Already Up To Date
            end
        end
        return true, resultTable
    else
        return false, 2, 1 -- 2 means Incorrect Argument, third parameter defines which
    end
end

GERTUpdaterAPI.InstallUpdate = function (moduleName)
    local parsedData = ParseSafeList()
    if not moduleName then
        return false, 1 -- 1 means no Arg 1
    elseif not parsedData[moduleName] then
        return false, 2 -- 2 means there's no local update to install
    end
    local success = fs.copy(parsedData[moduleName][2],parsedData[moduleName][1])
    if success then
        RemoveFromSafeList(moduleName)
        return true, 0
    else
        return false, 3 -- 3 means unknown error
    end
end

GERTUpdaterAPI.InstallStatus = function(moduleName)
    local parsedData = ParseSafeList()
    if not moduleName then
        return parsedData
    elseif type(moduleName) == "table" then
        local returnList = {}
        for k,name in pairs(moduleName) do
            returnList[name] = parsedData[name]
        end
        return returnList
    else
        return parsedData[moduleName]
    end
end

local function InstallEventHandler (event,moduleName)
    event.push("InstallModule",moduleName,GERTUpdaterAPI.InstallUpdate(moduleName))
end


GERTUpdaterAPI.InstallNewModule = function(moduleName)
    local config, parsedData = ParseConfig()
    if parsedData[moduleName] then
        return false, 4 -- 4 means module already installed. 
    end
    print(moduleName)
    local result = table.pack(GERTUpdaterAPI.DownloadUpdate(moduleName))
    if result == true then
        parsedData[moduleName] = moduleFolder .. "moduleName"
        writeConfig(config,parsedData)
        AddToSafeList(moduleName,parsedData[moduleName],cacheFolder .. moduleName,false)
        return GERTUpdaterAPI.InstallUpdate(moduleName)
    else
        return result
    end
end

GERTUpdaterAPI.UninstallModule = function(moduleName)
    local config, StoredPaths = ParseConfig()
    if not StoredPaths[moduleName] then
        return false
    end
    fs.remove(StoredPaths[moduleName])
    RemoveFromSafeList(moduleName)
    StoredPaths[moduleName] = nil
    writeConfig(config,StoredPaths)
    return true
end

GERTUpdaterAPI.AutoUpdate = function()
    return config["AutoUpdate"]
end

GERTUpdaterAPI.ChangeConfigSetting = function(setting,newValue)
    local config,storedPaths = ParseConfig()
    config[setting] = newValue
    local configFile = io.open(configPath,"r")
    local _ = configFile:read("*l")
    local tempConfig = configFile:read("*a")
    configFile:close()
    local configFile = io.open(configPath,"w")
    configFile:write(srl.unserialize(config) .. "\n")
    configFile:write(tempConfig)
    configFile:close()
    return true
end

GERTUpdaterAPI.UpdateAllInCache = function()
    local parsedData = ParseSafeList()
    local resultTable = {}
    for moduleName,moduleInformation in pairs(parsedData) do
        resultTable[moduleName] = GERTUpdaterAPI.InstallUpdate(moduleName)
    end
    return resultTable
end


event.listen("InstallReady",InstallEventHandler)
return GERTUpdaterAPI