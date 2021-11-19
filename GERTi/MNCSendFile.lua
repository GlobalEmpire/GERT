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

local function SearchForCache(data)
    local completeModuleCachePath = moduleCachePath .. fs.name(data[2]) .. "/" .. fs.name(data[3])
    if fs.exists(completeModuleCachePath) then
        return completeModuleCachePath
    else
        return false
    end
end

local function SendCachedFile(originAddress,data)
    local completeModuleCachePath = moduleCachePath .. fs.name(data[2]) .. "/" .. fs.name(data[3])
    local fileToSend = io.open(completeModuleCachePath)
    local chunk = fileToSend:read(8000)
    updateSockets[originAddress]:write(chunk)
    while string.len(chunk) == 8000 do
            
    end

end










local function HandleData(_,originAddress,connectionID,data)
    if connectionID == updatePort and updateSockets[originAddress] then
        data = updateSockets[originAddress]:read()
        if type(data[1]) == "table" then 
            if data[1] == "ModuleUpdate" then
                updateSockets[originAddress]:write("U.RequestReceived",data[3])
                local success, information = SearchForProgram(data[1])
                os.sleep(0.1)
                if not(success) then
                    if information == 2 then
                        updateSockets[originAddress]:write(false,2)
                    elseif information == 1 then
                        information = searchForCache(data[1])
                        if information then
                            updateSockets[originAddress]:write(true,fs.size(information))
                        end
                    end
                end
            elseif data[1] == "RequestCache" then
                SendCachedFile(originAddress,data)
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