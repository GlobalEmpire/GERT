local event = require("event")
local fs = require("filesystem")
local srl = require("serialization")

local UpdaterAPI = {}

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

local function RemoveFromSafeList (moduleName)
    if not moduleName then
        return false
    end
    local parsedData = ParseSafeList()
    fs.remove(parsedData[moduleName][2])
    parsedData[moduleName] = nil
    local file = io.open("/.SafeUpdateCache.srl", "w")
    file:write(srl.serialize(parsedData))
    file:close()
    return true
end

local function InstallUpdate (moduleName,parsedData)
    local success = filesystem.copy(parsedData[moduleName][2],parsedData[moduleName][1])
    if success then
        return moduleName
    else
        parsedData[moduleName][3] = 2
        return false
    end
end

UpdaterAPI.start = function()
    local parsedData = ParseSafeList()
    for name, Information in pairs(parsedData) do
        RemoveFromSafeList(InstallUpdate(name,parsedData))
    end
end

