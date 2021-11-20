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

local function InstallUpdate (moduleName,parsedData)
    local success = filesystem.copy(parsedData[moduleName][1],parsedData[moduleName][2])
    if success then
        parsedData[moduleName] = nil
    else
        parsedData[moduleName][3] = 2
    end
end

UpdaterAPI.start = function()
    local parsedData = ParseSafeList()
    for name, Information in pairs(parsedData) do
        InstallUpdate(name,parsedData)
    end
end

