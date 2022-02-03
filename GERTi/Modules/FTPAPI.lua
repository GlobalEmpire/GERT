local event = require("event")
local FTPCore = require("FTPCore")
local GERTi = require("GERTiClient")
local SRL = require("serialization")
local fs = require("filesystem")

--Error Codes
local INVALIDARGUMENT = 0
local NOSOCKET = -1
local TIMEOUT = -2
local NOSPACE = -3
local INTERRUPTED = -4
local NOLOCALFILE = -5
local UNKNOWN = -6
local SERVERRESPONSEFALSE = -7
local NOREMOTEFILE = -8
local CANNOTCHANGE = -9
local UPTODATE = -10
local NOREMOTERESPONSE = -11
local STUCK = -12

--Op Codes
local ALLGOOD = 0
local DOWNLOADED = 1
local ALREADYINSTALLED = 10

local CreateValidSocket = function (FileDetails)
    local socket = GERTi.openSocket(FileDetails.address,FileDetails.port)
    local serverPresence = false
    if socket then serverPresence = event.pullFiltered(5,function (eventName,oAdd,CID) return eventName=="GERTConnectionID" and oAdd==FileDetails.address and CID==FileDetails.port end) end
    if not serverPresence then
        socket:close()
        return false, NOSOCKET
    end
    return true, socket
end

local FTPAPI = {}
FTPAPI.CheckData = function (FileDetails,StepComplete,socket)
    if not StepComplete then
        return false, socket
    end
    socket:write("FTPGETFILEDATA",FileDetails.file,FileDetails.user,FileDetails.auth)
    local response = event.pullFiltered(5,function (eventName) return eventName=="modem_message" and #socket:read("-k")~=0 end)
    if not response then
        socket:close()
        return false, TIMEOUT
    end
    local data = socket:read()
    if data[1] == FileDetails.file then
        return data -- In sequence: {File name as passed as parameter | File Size}
    else
        return false, UNKNOWN
    end
end

FTPAPI.ProbeForSend = function (FileDetails, StepComplete, socket)
    if not StepComplete then
        return false, socket
    end
    local fileData = {}
    fileData.insert(fs.size(FileDetails.file))
    socket:write("FTPSENDPROBE",SRL.serialize(fileData))
    local success = event.pullFiltered(5, function (eventName, iAdd, dAdd, CID) if (iAdd == FileDetails.address or dAdd == FileDetails.address) and (dAdd == FileDetails.port or CID == FileDetails.port) then if eventName == "GERTConnectionClose" or eventName == "GERTData" then return true end end return false end)
    if success == "GERTConnectionClose" then
        return false, INTERRUPTED
    elseif not success then
        return false, TIMEOUT
    end
    while #socket:read("-k") == 0 do
        os.sleep() -- \\\\This might need a lengthening to 0.1, if OC is weird.\\\\
    end
    local returnData = socket:read()
    if returnData[1] == "FTPREADYTORECEIVE" then
        return true, returnData
    else
        return false, returnData
    end
end

FTPAPI.DownloadFile = function (FileDetails)-- Move to API Example
    --[[parameters = {"file","destination","address","port","user","auth"}
    file: obligatory; is the relative File Path as is stored in whatever is sending the file (taking into account scope)
    destination: obligatory; where the file will end up on the downloading device. NOTE! The destination must be a valid file location, or the program will error. There is no default!
    address: obligatory; this is the connector's address
    ]]
    local StepComplete,socket = CreateValidSocket(FileDetails)
    local FileData,result = FTPAPI.CheckData(FileDetails,StepComplete,socket)
    local StepComplete,result = FTPCore.DownloadFile(FileDetails,FileData,result or socket)
    if socket.close then
        socket:close()
    end
    return StepComplete,result
end

FTPAPI.UploadFile = function (FileDetails)-- Move to API Example
    local StepComplete,socket = CreateValidSocket(FileDetails)
    local StepComplete,result = FTPAPI.ProbeForSend(FileDetails, StepComplete, socket) -- result here contains "user" and "auth" from the other side. Use for verification?
    local StepComplete,result = FTPCore.UploadFile(FileDetails, StepComplete, (StepComplete and socket) or result)
    if socket.close then
        socket:close()
    end
    return StepComplete,result
end


FTPAPI.DownloadDaemon = function (timeout,port,address)-- Move to API Example
    local success, eventAddress, eventPort = event.pull(timeout,"GERTConnectionID",address,port)
    if not success then
        return false, TIMEOUT
    end
    local socket = GERTi.openSocket(eventAddress,eventPort) -- here i would add an event for "Connection Established" if I were integrating an API
    repeat
        success = event.pullFiltered(function (eventName, iAdd, dAdd, CID) if (iAdd == eventAddress or dAdd == eventAddress) and (dAdd == eventPort or CID == eventPort) then if eventName == "GERTConnectionClose" or eventName == "GERTData" then return true end end return false end)
    until success == "GERTConnectionClose" or socket:read("-k")[1] == "FTPSENDPROBE"
    if not success then
        return false, TIMEOUT
    elseif success == "GERTConnectionClose" then
        socket:close()
        return false, INTERRUPTED
    end
    local socketData = socket:read()
    socketData[2] = SRL.unserialize(socketData[2])
end
