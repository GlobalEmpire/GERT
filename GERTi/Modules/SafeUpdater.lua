-- GCU Boot Component - Beta 1 
local event = require("event")
local fs = require("filesystem")
local srl = require("serialization")
local term = require("term")
local computer = require("computer")

local UpdaterAPI = {}
local width = term.getViewport()

local function bootBeep(freq,rep)
    if not (args == "mute") then
        computer.beep(freq,rep)
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
    local success = fs.copy(parsedData[moduleName][2],parsedData[moduleName][1])
    if success then
        return moduleName
    else
        parsedData[moduleName][3] = 2
        return false
    end
end

function start()
    term.clear()
    bootBeep(1000)
    io.stdout:write("Parsing GERTUpdate SafeFile\n")
    local parsedData = ParseSafeList()
    if not parsedData then
        os.sleep(0.5)
        io.stdout:write("No Updates Queued\n")
        bootBeep(2000)
        os.sleep(2)
        io.stdout:write("Resuming Boot\n")
        os.sleep(0.3)
        return
    end
    io.stdout:write("Updates Found\n")
    bootBeep(1500)
    os.sleep(0.3)
    io.stderr:write("INITIATING PRE-SHELL SECURE UPDATE DAEMON\n")
    os.sleep(0.3)
    io.stderr:write(string.rep("╤",width))
    io.stderr:write(string.rep("╧",width))
    for name, Information in pairs(parsedData) do
        io.stdout:write("Installing Module: <".. tostring(name) .. ">.....")
        if RemoveFromSafeList(InstallUpdate(name,parsedData),parsedData) then
            io.stdout:write(" Success!\n")
            bootBeep(1500)
        else
            io.stderr:write(" Failure!\n")
            bootBeep(1000)
        end
    end
    io.stderr:write("SECURE UPDATE COMPLETE\n")
    bootBeep(1500)
    bootBeep(2000)
    os.sleep(0.3)
    io.stderr:write(string.rep("╤",width))
    io.stderr:write(string.rep("╧",width))
    os.sleep(0.2)
    io.stdout:write("Resuming Boot\n")
end

