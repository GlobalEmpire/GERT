local event = require("event")
local fs = require("filesystem")
local srl = require("serialization")
local term = require("term")

local UpdaterAPI = {}

local function checkTerm (text)
    if term.isAvailable() then
        term.write(text,
    end
end

local function ParseSafeList ()
    if not fs.exists("/.SafeUpdateCache.srl") then 
        return false
    end
    local file = io.open("/.SafeUpdateCache.srl", "r")
    local rawData = file:read("*a")
    file:close()
    local parsedData = srl.unserialize(rawData)
    return parsedData or false
end

local function RemoveFromSafeList (moduleName,parsedData)
    if not moduleName then
        return false
    end
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
    io.stdout:write("Parsing GERTUpdate SafeFile\n")
    local parsedData = ParseSafeList()
    if not parsedData then
        io.stdout:write("No Updates Queued\n")
        os.sleep(0.5)
        return
    end
    io.stdout:write("Updates Found\n")
    os.sleep(0.3)
    io.stderr:write("INITIATING PRE-SHELL SECURE UPDATE PROGRAM")
    gpu.getResolution
    for name, Information in pairs(parsedData) do
        RemoveFromSafeList(InstallUpdate(name,parsedData),parsedData)
    end
end

