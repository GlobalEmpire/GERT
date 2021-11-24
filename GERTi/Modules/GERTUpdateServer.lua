-- GUS Server Component - Beta 1
local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local srl = require("serialization")

local updatePort = 941
local updateSockets = {}
local mainRemoteDirectory = "https://raw.githubusercontent.com/leothehero/GERT/Development/GERTi/Modules/" --"https://raw.githubusercontent.com/GlobalEmpire/GERT/Development/GERTi/Modules/"
local configPath = "/etc/GERTUpdateServer.cfg"
local loadableModulePath = "/usr/lib/"
local unloadableModulePath = "/modules/"
local config = {}
local storedPaths = {}
local GERTUpdaterAPI = {}

if not fs.isDirectory(loadableModulePath) then
    fs.makeDirectory(loadableModulePath)
end
if not fs.isDirectory(unloadableModulePath) then
    fs.makeDirectory(unloadableModulePath)
end

local function CreateConfigFile ()
    local file = io.open(configPath, "w")
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
    storedPaths["GERTUpdateServer.lua"] = storedPaths["GERTUpdateServer.lua"] or "/usr/lib/GERTUpdateServer.lua"
    configFile:write(srl.serialize(config) .. "\n")
    for name,path in pairs(storedPaths) do
        configFile:write(name .. "|" .. path .. "\n")
    end
    configFile:close()
end

local function ParseConfig ()
    local configFile = io.open(configPath, "r")
    config = srl.unserialize(configFile:read("*l"))
    local lineData = configFile:read("*l")
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
    configFile:close()
    return config,storedPaths
end

local function CompleteSocket(_,originAddress,connectionID)
    if connectionID == updatePort then
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
        return false, 4 -- 4 means module already installed. 
    end
    storedPaths[moduleName] = modulePath
    writeConfig(config,storedPaths)
    return true
end

GERTUpdaterAPI.RemoveModule = function(moduleName)
    local config, storedPaths = ParseConfig()
    if not storedPaths[moduleName] or moduleName == "GERTiMNC.lua" or moduleName == "GERTiClient.lua" or moduleName == "MNCAPI.lua" or moduleName == "GERTUpdateServer.lua" then
        return false
    end
    fs.remove(storedPaths[moduleName])
    storedPaths[moduleName] = nil
    writeConfig(config,storedPaths)
    return true
end

GERTUpdaterAPI.CheckLatest = function(moduleName)
    local config, storedPaths = ParseConfig()
    if not storedPaths[moduleName] then
        return false, 2 -- 2 means it is not set up to update this module
    end
    local versionHeader = ""
    local localCacheExists = fs.exists(storedPaths[moduleName])
    local fileStorage = ""
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
            if fileLen < remainingSpace-200 then
                local CacheFile = io.open(storedPaths[moduleName],"w")
                CacheFile:write(fullFile)
                CacheFile:close()
                return true, -1, versionHeader -- this means that the file was downloaded
            else
                return false, 3 -- 3 means Insufficient Space To Download File On MNC. Contact Admin if returned
            end
        end
        return true, 0, versionHeader -- 0 means up to date
    else
        local eCode = 1
        if localCacheExists then
            eCode = -2
        end
        return localCacheExists, eCode, versionHeader -- 1 means that it could not establish a connection. This line here will return true if the cache file exists, false otherwise
    end
end

local function SendCachedFile (originAddress,moduleName) -- returns true if successful, false if timeout
    local config, storedPaths = ParseConfig()
    local fileToSend = io.open(storedPaths[moduleName], "rb")
    local chunk = fileToSend:read(8000)
    while chunk do
        updateSockets[originAddress]:write(chunk)
        local success = event.pull(10, "GERTData", originAddress, updatePort)
        if not success then
            updateSockets[originAddress]:close()
            fileToSend:close()
            return false
        end
        chunk = fileToSend:read(8000)
    end
    fileToSend:close()
    os.sleep(0.5)
    updateSockets[originAddress]:close()
    updateSockets[originAddress] = nil
    return true
end

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
                local success, information = pcall(SendCachedFile(originAddress,data[1][2]))
                if not success then
                    event.push("GERTupdater Send Error", information)
                end
            end
        end
    end
end

if fs.exists(configPath) then
    ParseConfig()
else
    CreateConfigFile()
end

GERTUpdaterAPI.listeners = {}

GERTUpdaterAPI.StartHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.listen("GERTConnectionID",CompleteSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.listen("GERTConnectionClose",CloseSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.listen("GERTData",HandleData)    
end
GERTUpdaterAPI.StopHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.ignore("GERTConnectionID",CompleteSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.ignore("GERTConnectionClose",CloseSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.ignore("GERTData",HandleData)
end

GERTUpdaterAPI.StartHandlers()

return GERTUpdaterAPI