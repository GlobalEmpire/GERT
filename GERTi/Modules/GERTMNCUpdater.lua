local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local srl = require("serialization")

local updatePort = 941
local updateSockets = {}
local mainRemoteDirectory = "https://raw.githubusercontent.com/GlobalEmpire/OC-Programs/master/GERTiModules/"
local configPath = "/etc/GERTUpdater.cfg"
local config = {}
local storedPaths = {}
local GERTUpdaterAPI = {}

local function CreateConfigFile ()
    local file = io.open(configPath, "w")
    file:write(srl.serialize(config))
    file:close()
end

if fs.exists(configPath) then
    local configFile = io.open(configPath,"r")
    config = srl.deserialize(configFile:read("*l"))
    local tempPath = configFile:read("*l")
    while tempPath ~= "" do
        storedPaths[fs.name(tempPath)] = tempPath
        tempPath = configFile:read("*l")
    end
    configFile:close()
else
    CreateConfigFile()
end

local function CompleteSocket(_,originAddress,connectionID)
    if connectionID == updatePort then
        updateSockets[originAddress] = GERTi.openSocket(originAddress,connectionID)
    end
    return true
end

local function CloseSocket(_, originAddress,destAddress,ConnectionID)
    if updateSockets[originAddress] then
        updateSockets[originAddress]:close()
        updateSockets[originAddress] = nil
    elseif updateSockets[destAddress] then 
        updateSockets[destAddress]:close()
        updateSockets[destAddress] = nil
    end
    return true
end

GERTUpdaterAPI.CheckLatest = function(data)
    if not storedPaths[data[2]] then
        return false, 2 -- 2 means it is not set up to update this module
    end
    local versionHeader = ""
    local localCacheExists = fs.exists(storedPaths[data[2]])
    if localCacheExists then
        local file = io.open(storedPaths[data[2]], "r")
        versionHeader = file:read("*l")
        file:close()
    end
    local completeRemoteURL = mainRemoteDirectory .. data[2]
    local success, fullFile = pcall(function() 
        local remoteFile = internet.request(completeRemoteURL)
        local fullFile = ""
        for chunk in remoteFile do
            fullFile = fullFile .. chunk
        end 
        return fullFile
    end)
    if success then
        local remoteVersionHeader = fullFile:gmatch("[^\n]+")
        if remoteVersionHeader ~= versionHeader then 
            local storageDrive = fs.get(storedPaths[data[2]])
            local remainingSpace = fs.size(storedPaths[data[2]]) + (storageDrive.spaceTotal()-storageDrive.spaceUsed())
            fileLen = string.len(tempFileDownload)
            if fileLen < remainingSpace-200 then
                local CacheFile = io.open(storedPaths[data[2]],"w")
                CacheFile:write(tempFileDownload)
                CacheFile:close()
                return true, -1, versionHeader -- this means that the file was downloaded
            else
                return false, 3 -- 3 means Insufficient Space To Download File On MNC. Contact Admin if returned
            end
        end
        return true, 0, versionHeader -- 0 means no issues
    else
        return localCacheExists, 1, versionHeader -- 1 means that it could not establish a connection. This line here will return true if the cache file exists, false otherwise
    end
end

GERTUpdaterAPI.SendCachedFile = function(originAddress,data) -- returns true if successful, false if timeout
    local fileToSend = io.open(storedPaths[data[2]], "rb")
    local chunk = fileToSend:read(8000)
    updateSockets[originAddress]:write(chunk)
    while string.len(chunk) == 8000 do
        local success = event.pull(10, "GERTData", originAddress, updatePort)
        if not success then
            updateSockets[originAddress]:close()
            fileToSend:close()
            return false
        elseif socket:read()[1] == "U.Continue"

        end
        chunk = fileToSend:read(8000)
        updateSockets[originAddress]:write(chunk)
    end
    fileToSend:close()
    os.sleep(0.5)
    updateSockets[originAddress]:close()
    updateSockets[originAddress] = nil
    return true
end


local function HandleData(_,originAddress,connectionID,data)
    if connectionID == updatePort and updateSockets[originAddress] then
        data = updateSockets[originAddress]:read()
        if type(data[1]) == "table" then 
            if data[1][1] == "ModuleUpdate" then
                updateSockets[originAddress]:write("U.RequestReceived",data[2])
                local downloadFile, errorState, versionHeader = GERTUpdaterAPI.CheckLatest(data[1])
                if downloadFile then
                    updateSockets[originAddress]:write(true,fs.size(storedPaths[data[1][2]]),errorState,versionHeader) -- Error state: 0 and lower=OK, 1=NOFILEONREMOTE, 2=INVALIDMODULE, 3=INSUFFICIENTMNCSPACE
                else
                    updateSockets[originAddress]:write(false,errorState)
                end
            elseif data[1] == "RequestCache" then
                local success, information = pcall(GERTUpdaterAPI.SendCachedFile(originAddress,data))
                if not success then
                    event.push("GERTupdater Send Error", information)
                end
            end
        end
    end
end


GERTUpdaterAPI.listeners = {}

GERTUpdaterAPI.startHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.listen("GERTConnectionID",CompleteSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.listen("GERTConnectionClose",CloseSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.listen("GERTData",HandleData)    
end
GERTUpdaterAPI.stopHandlers = function()
    GERTUpdaterAPI.listeners.GERTUpdateSocketOpenerID = event.ignore("GERTConnectionID",CompleteSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketCloserID = event.ignore("GERTConnectionClose",CloseSocket)
    GERTUpdaterAPI.listeners.GERTUpdateSocketHandlerID = event.ignore("GERTData",HandleData)
end

return GERTUpdaterAPI