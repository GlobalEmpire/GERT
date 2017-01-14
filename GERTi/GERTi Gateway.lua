-- Under Construction
local component = require("component")
local event = require("event")
local serialize = require("serialization")

local modem = component.modem
if component.isAvailable("tunnel") then
    local tunnel = component.tunnel
end

local childNodes = {}
local childNum = 1
local tier = 0

-- functions to store the children and then sort the table
local function storeChild(eventName, receivingModem, sendingModem, port, distance, package)
    -- register neighbors for communication to gateway
    childNodes[childNum] = {}
    childNodes[childNum]["address"] = sendingModem
    childNodes[childNum]["tier"] = tonumber(package)
    childNodes[childNum]["port"] = tonumber(port)
    childNodes[childNum]["children"]={}
    childNum = childNum + 1
end
local function sortTable(elementOne, elementTwo)
    if tonumber(elementOne["tier"]) < tonumber(elementTwo["tier"]) then
        return true
    else
        return false
    end
end
local function transmitInformation(sendTo, port, ...)
    if port ~= 0 then
        modem.send(sendTo, port, ...)
    else
        tunnel.send(...)
    end
end
local function receivePacket(eventName, receivingModem, sendingModem, port, distance, ...)
    local doesExist = false
    if ... == "GERTiStart" then
        for key,value in pairs(childNodes) do
            if value["address"] == sendingModem then
                doesExist = true
                childNodes[key]["tier"] = ...
                childNodes[key]["port"] = port
                childNodex[key]["children"] = {}
                break
            end
        end
        if doesExist == false then
            storeChild(eventName, receivingModem, sendingModem, port, distacnce, ...)
        end
        transmitInformation(sendingModem, port, tier)
    end
end
event.listen("modem_message", receivePacket)