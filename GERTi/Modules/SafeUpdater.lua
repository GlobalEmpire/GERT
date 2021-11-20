local event = require("event")
local fs = require("filesystem")
local shell = require("shell")
local srl = require("serialization")

local args, opts = shell.parse(...)

local UpdaterAPI = {}

local function checkAutorun()
    local header = ""
    local path = "/.autorun"
    local autorunExists = fs.exists(path)
    if autorunExists then
        local file = io.open(path, "r")
        header = file:read("*l")
        file:close()
    else
        local file = io.open(path, "w")
        file:write("require('SafeUpdater.lua').UpdateAllInCache()")
        file:close()
        if fs.
    end

end

local function ParseSafeList ()
    if not fs.exists("/.SafeUpdateCache") then 
        return {}
    end
    local file = io.open("/.SafeUpdateCache", "r")
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
    local file = io.open("/.SafeUpdateCache", "w")
    file:write(srl.serialize(parsedData))
    file:close()
    return true
end


UpdaterAPI.InstallUpdate = function (event, moduleName)
    if not moduleName then
        moduleName = event
    end
end


UpdaterAPI.Register = function (moduleName,currentPath,cachePath,installWhenReady)
    local error, success = pcall(AddToSafeList(moduleName,currentPath,cachePath,installWhenReady))
    if not(error and success) then
        return error, success
    end
    if installWhenReady then
        event.push("UpdateAvailable",moduleName)
    end
end


UpdaterAPI.UpdateAllInCache = function()

end

UpdaterAPI.InstallStatus = function(moduleName)
    local parsedData = ParseSafeList()
    if not moduleName then
        return parsetData
    elseif type(moduleName) == "table" then
        local returnList = {}
        for k,name in moduleName do
            returnList[name] = parsedData[name]
        end
        return returnList
    else
        return parsedData[moduleName]
    end
end

local function InstallEventHandler (event,moduleName)
    UpdaterAPI.InstallUpdate(moduleName)
end


event.listen("InstallReady",InstallEventHandler)

return UpdaterAPI