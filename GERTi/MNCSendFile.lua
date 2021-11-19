local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")
local event = require("event")
local srl = require("serialization")

local updatePort = 941
local updateSockets = {}
local mainRemoteDirectory = "https://raw.githubusercontent.com/GlobalEmpire/OC-Programs/master/GERTiModules/"
local moduleCachePath = "/ModuleCache/"
local configPath = "/etc/GERTUpdater.cfg"
local config = {}
local configPaths = {}

if fs.exists(configPath) then
    local configFile = io.open(configPath,"r")
    config = srl.deserialize(configFile:read("*l"))
    local tempPath = configFile:read("*l")
    while tempPath ~= "" do
        configPaths[fs.name(tempPath)] = tempPath
        tempPath = configFile:read("*l")
    end
    configFile:close()
else
    createConfigFile()
end


local function CompleteSocket(_,originAddress,connectionID)
    if connectionID == updatePort then
        updateSockets[originAddress] = GERTi.openSocket(originAddress,connectionID)
    end
end

local function checkLatest(data)
    if not configPaths[data[2]] then
        return false, 2 -- 2 means it is not set up to update this module
    end
    local versionHeader = ""
    local localCacheExists = fs.exists(configPaths[data[2]])
    if localCacheExists then
        local file = io.open(configPaths[data[2]], "r")
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
            local file = io.open(configPaths[data[2]], "w")
        end
        return true, 0 -- 0 means no issues
    else
        return localCacheExists, 1 -- 1 means that it could not establish a connection. This line here will return true if the cache file exists, false otherwise
    end
end


local function SearchForProgram(data)
    local completeRemoteURL = mainRemoteDirectory .. fs.name(data[2]) .. "/latest/" .. fs.name(data[3])
    local remoteFile = internet.request(completeRemoteURL)
    local succeed,tempFileDownload = pcall(function()
        local tempFileDownload = ""
        for fileChunk in remoteFile do
            tempFileDownload = tempFileDownload .. fileChunk
        end
        return tempFileDownload
    end)
    if not succeed then
        --Here would be where we initiate the check for a (mainRemoteDirectory .. ".removed") check
        remoteFile:close()
        return false, 1 -- 1 means file not found on remote
    end
    local storageDrive = fs.get(moduleCachePath)
    local completeModuleCachePath = moduleCachePath .. fs.name(data[2]) .. "/" .. fs.name(data[3])
    local remainingSpace = fs.size(completeModuleCachePath) + (storageDrive.spaceTotal()-storageDrive.spaceUsed())
    fileLen = string.len(tempFileDownload)
    if fileLen < remainingSpace-200 then
        local CacheFile = io.open(completeModuleCachePath,"w")
        CacheFile:write(tempFileDownload)
        CacheFile:close()
        return true, completeModuleCachePath
    else
        return false, 2 -- 2 means Insufficient Space To Download File On MNC. Contact Admin if returned
    end
end

local function SendCachedFile(originAddress,data)
    local fileToSend = io.open(configPaths[data[2]])
    local chunk = fileToSend:read(8000)
    updateSockets[originAddress]:write(chunk)
    while string.len(chunk) == 8000 do
        event.pull(10, "GERTData", originAddress, updatePort)
        chunk = fileToSend:read(8000)
        updateSockets[originAddress]:write(chunk)
    end
    updateSockets[originAddress]:close()
    updateSockets[originAddress] = nil
    return true
end

local function HandleData(_,originAddress,connectionID,data)
    if connectionID == updatePort and updateSockets[originAddress] then
        data = updateSockets[originAddress]:read()
        if type(data[1]) == "table" then 
            if data[1] == "ModuleUpdate" then
                updateSockets[originAddress]:write("U.RequestReceived",data[2])
                local downloadFile, errorState = checkLatest(data[1])
                if downloadFile then
                    SendCachedFile(originAddress,data[1])
                end
                if cachePath then
                    updateSockets[originAddress]:write(true,fs.size(information))
                end
                
                
                
                local success, information = SearchForProgram(data[1])
                os.sleep(0.1)
                if not success then
                    if information == 2 then
                        updateSockets[originAddress]:write(false,2)
                    elseif information == 1 then
                        
                    end
                end
            elseif data[1] == "RequestCache" then
                local success, information = pcall(SendCachedFile(originAddress,data))
                if not success then
                    event.push
                end
            end
        end
    end
end

local ModuleFolder = ""
local IDFilePath = "/moduleListeners"

local SendFile = function (address,filename)

end

local MNCUpdateListenerSocketID = event.listen("GERTConnectionID",CompleteSocket)



if fs.exists(IDFilePath) and fs.isDirectory(IDFilePath) then
    file 
event.register