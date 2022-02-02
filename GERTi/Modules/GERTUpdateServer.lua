-- GUS Server Component - The Cleanliness Update|Release 1.2
local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local srl = require("serialization")

local updatePort = 941
local updateSockets = {}
local mainRemoteDirectory = "https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/"
local configPath = "/etc/GERTUpdateServer.cfg"
local loadableModulePath = "/usr/lib/"
local unloadableModulePath = "/modules/"
local GERTUpdaterAPI = {}

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

if not fs.isDirectory(loadableModulePath) then
    fs.makeDirectory(loadableModulePath)
end
if not fs.isDirectory(unloadableModulePath) then
    fs.makeDirectory(unloadableModulePath)
end

local function CreateConfigFile ()
    local file = io.open(configPath, "w")
    local config = {}
    config.DailyCheck = true
    config.StartImmediately = true
    config.SETDNS = true
    file:write(srl.serialize(config))
    file:close()
end

if not fs.exists(configPath) then
    CreateConfigFile()
end

local function writeConfig (config,storedPaths)
    local configFile = io.open(configPath,"w")
    storedPaths["GERTiClient.lua"] = nil
    storedPaths["MNCAPI.lua"] = nil
    storedPaths["GERTiMNC.lua"] = nil
    configFile:write(srl.serialize(config))
    for name,path in pairs(storedPaths) do
        configFile:write("\n" ..name .. "|" .. path)
    end
    configFile:close()
end

local function ParseConfig()
    local configFile = io.open(configPath, "r")
    local config = srl.unserialize(configFile:read("*l"))
    local lineData = configFile:read("*l")
    local storedPaths = {}
    while lineData ~= "" and lineData ~= nil do
        local temporaryDataTable = {}
        for element in string.gmatch(lineData, "([^".."|".."]+)") do
            table.insert(temporaryDataTable,element)
        end
        if #temporaryDataTable > 1 then
            storedPaths[fs.name(temporaryDataTable[1])] = temporaryDataTable[2]
        else
            storedPaths[fs.name(temporaryDataTable[1])] = temporaryDataTable[1]
        end
        lineData = configFile:read("*l")
    end
    storedPaths["GERTiMNC.lua"] = "/etc/rc.d/GERTiMNC.lua"
    storedPaths["GERTiClient.lua"] = "/modules/GERTiClient.lua"
    storedPaths["MNCAPI.lua"] = "/lib/GERTiClient.lua"
    storedPaths["GERTUpdateServer.lua"] = storedPaths["GERTUpdateServer.lua"] or "/usr/lib/GERTUpdateServer.lua"
    configFile:close()
    return config,storedPaths
end

local function CompleteSocket(_,originAddress,connectionID)
    if connectionID == updatePort then
        os.sleep()
        updateSockets[originAddress] = GERTi.openSocket(originAddress,connectionID)
    end
    return true
end

local function CloseSocket(_, originAddress,destAddress)
    if updateSockets[originAddress] then
        updateSockets[originAddress]:close()
        updateSockets[originAddress] = nil
    elseif updateSockets[destAddress] then 
        updateSockets[destAddress]:close()
        updateSockets[destAddress] = nil
    end
    return true
end

GERTUpdaterAPI.SyncNewModule = function(moduleName,modulePath)
    local config, storedPaths = ParseConfig()
    moduleName = fs.name(moduleName)
    if not modulePath then
        modulePath = loadableModulePath..moduleName
    end
    if storedPaths[moduleName] then
        return true, ALREADYINSTALLED
    end
    storedPaths[moduleName] = modulePath
    writeConfig(config,storedPaths)
    return true, ALLGOOD
end

GERTUpdaterAPI.RemoveModule = function(moduleName)
    local config, storedPaths = ParseConfig()
    if not storedPaths[moduleName] or moduleName == "GERTiMNC.lua" or moduleName == "GERTiClient.lua" or moduleName == "MNCAPI.lua" or moduleName == "GERTUpdateServer.lua" then
        return false, CANNOTCHANGE
    end
    fs.remove(storedPaths[moduleName])
    storedPaths[moduleName] = nil
    writeConfig(config,storedPaths)
    return true, ALLGOOD
end

GERTUpdaterAPI.CheckLatest = function(moduleName, config, storedPaths)
    if not config then
        config, storedPaths = ParseConfig()
    end
    if not storedPaths[moduleName] then
        return false, MODULENOTCONFIGURED
    end
    local versionHeader = ""
    local localCacheExists = fs.exists(storedPaths[moduleName])
    if localCacheExists then
        local file = io.open(storedPaths[moduleName], "r")
        versionHeader = tostring(file:read("*l"))
        file:close()
    end
    local completeRemoteURL = mainRemoteDirectory .. moduleName
    if moduleName == "GERTiMNC.lua" or moduleName == "GERTiClient.lua" then
        completeRemoteURL = mainRemoteDirectory .. "../" .. moduleName
    end
    local success, fullFile = pcall(function() 
        local remoteFile = internet.request(completeRemoteURL)
        local fullFile = ""
        for chunk in remoteFile do
            fullFile = fullFile .. chunk
        end 
        return fullFile
    end)
    if success then
        local remoteVersionHeader = fullFile:gmatch("[^\n]+")()
        if remoteVersionHeader ~= versionHeader then 
            local storageDrive = fs.get(storedPaths[moduleName])
            local remainingSpace = fs.size(storedPaths[moduleName]) + (storageDrive.spaceTotal()-storageDrive.spaceUsed())
            local fileLen = string.len(fullFile)
            if fileLen < remainingSpace-1000 then
                local CacheFile = io.open(storedPaths[moduleName],"w")
                CacheFile:write(fullFile)
                CacheFile:close()
                return true, DOWNLOADED, remoteVersionHeader -- The file was downloaded
            else
                return false, NOSPACE --Insufficient Space To Download File On MNC. Contact Admin if returned
            end
        end
        return true, ALLGOOD, versionHeader -- 0 means up to date
    else
        local eCode = NOSOCKET
        if localCacheExists then
            eCode = NOREMOTERESPONSE
        end
        return localCacheExists, eCode, versionHeader
    end
end

--[[local function SendCachedFile (originAddress,moduleName) -- returns true if successful, false if timeout or invalid modulename
    local config, storedPaths = ParseConfig()
    if storedPaths[moduleName] == nil then
        return false, MODULENOTCONFIGURED
    end
    local fileToSend = io.open(storedPaths[moduleName], "rb")
    local chunk = fileToSend:read(8000)
    while chunk ~= nil and chunk ~= "" do
        updateSockets[originAddress]:write(chunk)
        local success = event.pull(10, "GERTData", originAddress, updatePort)
        if not success then
            updateSockets[originAddress]:close()
            fileToSend:close()
            return false, TIMEOUT
        end
        chunk = fileToSend:read(8000)
    end
    fileToSend:close()
    os.sleep(0.5)
    updateSockets[originAddress]:close()
    updateSockets[originAddress] = nil
    return true, ALLGOOD
end]]

local function HandleData(_,originAddress,connectionID,data)
    local config, storedPaths = ParseConfig()
    if connectionID == updatePort and updateSockets[originAddress] then
        data = updateSockets[originAddress]:read()
        if type(data[1]) == "table" then 
            if data[1][1] == "ModuleUpdate" then
                updateSockets[originAddress]:write("U.RequestReceived",data[1][2])
                local downloadFile, errorState, versionHeader = GERTUpdaterAPI.CheckLatest(data[1][2])
                if downloadFile then
                    updateSockets[originAddress]:write(true,fs.size(storedPaths[data[1][2]]),errorState,versionHeader) -- Error state: 0 and lower=OK, 1=NOFILEONREMOTE, 2=INVALIDMODULE, 3=INSUFFICIENTMNCSPACE
                else
                    updateSockets[originAddress]:write(false,errorState)
                end
            elseif data[1][1] == "RequestCache" then
                local success, information = SendCachedFile(originAddress,data[1][2])
                if not success then
                    event.push("GERTupdater Send Error", information)
                end
            end
        end
    end
end

GERTUpdaterAPI.GetSetting = function(setting)
    local config,storedPaths = ParseConfig()
    return config[setting]
end

GERTUpdaterAPI.ChangeConfigSetting = function(setting,newValue)
    local config, storedPaths = ParseConfig()
    config[setting] = newValue
    writeConfig(config,storedPaths)
    return true
end


if fs.exists(configPath) then
    ParseConfig()
else
    CreateConfigFile()
end

GERTUpdaterAPI.listeners = {}

GERTUpdaterAPI.StartHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.listen("GERTConnectionID",CompleteSocket) or GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.listen("GERTConnectionClose",CloseSocket) or GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.listen("GERTData",HandleData) or GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID
    return GERTUpdaterAPI.listeners
end
GERTUpdaterAPI.StopHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.ignore("GERTConnectionID",CompleteSocket) or GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.ignore("GERTConnectionClose",CloseSocket) or GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.ignore("GERTData",HandleData) or GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID
    return GERTUpdaterAPI.listeners
end

local eventTimers = {}
GERTUpdaterAPI.StartTimers = function ()
    if eventTimers.hourly then
        event.cancel(eventTimers.hourly)
    end
    eventTimers.hourly = event.timer(3600,function () GERTUpdaterAPI.CheckLatest("GERTUpdateServer.lua") end, math.huge)
    if eventTimers.daily then
        event.cancel(eventTimers.daily)
    end
    eventTimers.daily = event.timer(86400,function () local config, storedPaths = ParseConfig() for k,v in pairs(storedPaths) do GERTUpdaterAPI.CheckLatest(k,config,storedPaths) end end, math.huge)
    return eventTimers
end

GERTUpdaterAPI.StopTimers = function ()
    if eventTimers.hourly then
        eventTimers.hourly = event.cancel(eventTimers.hourly)
    end
    if eventTimers.daily then
        eventTimers.daily = event.cancel(eventTimers.daily)
    end
    return eventTimers
end

local config,storedPaths = ParseConfig()
if config.StartImmediately then
    GERTUpdaterAPI.StartHandlers()
end
if config.DailyCheck then
    GERTUpdaterAPI.StartTimers()
end
if config.SETDNS then
    GERTi.updateDNSRecord("GUS",0.0)
end


return GERTUpdaterAPI