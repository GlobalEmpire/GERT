local GERTi = require("GERTiClient")
local fs = require("filesystem")
local internet = require("internet")

local updatePort = 941
local updateSockets = {}
local mainRemoteDirectory = "https://raw.githubusercontent.com/GlobalEmpire/OC-Programs/master/Programs/"
local moduleCachePath = "/.ModuleCache/"

local function CompleteSocket(_,originAddress,connectionID)
    if connectionID == updatePort then
        updateSockets[originAddress] = GERTi.openSocket(originAddress,connectionID)
    end
end

local function SearchForProgram(data)
    local completeRemoteURL = mainRemoteDirectory .. fs.name(data[2]) .. "/latest/" .. fs.name(data[3])
    local remoteFile = internet.request(completeRemoteURL)
    local tempFileDownload = remoteFile:read(100)
    if string.find("404: Not Found") then
        --Here would be where we initiate the check for a (mainRemoteDirectory .. ".removed") check
        remoteFile:close()
        return false, 1 -- 1 means file not found on remote
    end
    local CacheFile = io.open(moduleCachePath .. fs.name(data[2]) .. "/" .. fs.name(data[3]))
    CacheFile:write()
end












local function HandleData(_,originAddress,connectionID,data)
    if connectionID == updatePort and updateSockets[originAddress] then
        data = updateSockets[originAddress]:read()
        if type(data[1]) == "table" and data[1] == "ModuleUpdate" then
            updateSockets[originAddress]:write("U.RequestReceived",data[3])
            SearchForProgram(data[1])
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