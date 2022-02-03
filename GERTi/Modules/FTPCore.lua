-- FTP Core - Separation Update (Release 1)|R1
local fs = require("filesystem")
local GERTi = require("GERTiClient")
local event = require("event")
local SRL = require("serialization")

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

local FTPCore = {}


FTPCore.DownloadFile = function (FileDetails,FileData,socket) --Provide file as relative path on server, destination as where it goes, address of the server.
    if not FileData then
        return false, socket
    end
    local remoteSize = FileData
    if type(remoteSize) == "table" then
        remoteSize = remoteSize[2]
    end
    if remoteSize == 0 then
        return false, NOREMOTEFILE
    end
    local storageDrive = fs.get(FileDetails.destination)
    local remainingSpace = (storageDrive.spaceTotal()-storageDrive.spaceUsed())
    if remoteSize > remainingSpace - 1000 then
        return false, NOSPACE -- insufficient space for download
    end
    local destfile = io.open(FileDetails.destination, "wb")
    local loopDone = false
    socket:write("FTPREADYTORECEIVE",FileDetails.file,FileDetails.user,FileDetails.auth)
    repeat
        local response = event.pullFiltered(5, function (eventName, iAdd, dAdd, CID) if (iAdd == FileDetails.address or dAdd == FileDetails.address) and (dAdd == FileDetails.port or CID == FileDetails.port) then if eventName == "GERTConnectionClose" or eventName == "GERTData" then return true end end return false end)
        if not response then
            return false, TIMEOUT
        elseif response == "GERTConnectionClose" then
            if type(socket:read("-k")) == "table" and #socket:read("-k")[1] > 1 then
                destfile:write(socket:read()[1][2])
            end
            loopDone = INTERRUPTED
        elseif socket:read("-k")[1][1] == "FTPDATASENT" then
            destfile:write(socket:read()[1][2])
            socket:write("FTPREADYTOCONTINUERECEIVE")
        elseif socket:read("-k")[1][1] == "FTPDATAFIN" then
            loopDone = ALLGOOD
        end
    until loopDone
    destfile:close()
    return fs.size(FileDetails.destination) == remoteSize, loopDone
end


FTPCore.UploadFile = function (FileDetails,StepComplete,socket) -- returns true if successful, false if timeout or invalid modulename -- Switch to FTPCore
    --Do Auth Processing in a separate function and append it to destination. if not user then user = "public"
    if not StepComplete then
        return false, socket
    end
    local fileToSend = io.open(FileDetails.file, "rb")
    local chunk = fileToSend:read(8000)
    local sendState = "FTPDATASENT"
    local lastState = ALLGOOD
    local loopStuck = 0
    socket:read()
    while chunk ~= nil and chunk ~= "" do
        socket:write(sendState,chunk)
        local success = event.pullFiltered(5, function (eventName, iAdd, dAdd, CID) if (iAdd == FileDetails.address or dAdd == FileDetails.address) and (dAdd == FileDetails.port or CID == FileDetails.port) then if eventName == "GERTConnectionClose" or eventName == "GERTData" then return true end end return false end)
        if success == "GERTConnectionClose" then
            fileToSend:close()
            return false, INTERRUPTED
        end
        while success and #socket:read("-k") == 0 do
            os.sleep() -- \\\\This might need a lengthening to 0.1, if OC is weird.\\\\
        end
        local returnData = socket:read()[1]
        if not success then
            fileToSend:close()
            return false, TIMEOUT
        elseif returnData == "FTPREADYTOCONTINUERECEIVE" then
            loopStuck = 0
            sendState = "FTPDATASENT"
            chunk = fileToSend:read(8000)
        elseif returnData == "FTPREPEATDATA" and loopStuck < 5 then --FutureProofing
            sendState = "FTPREPEATINGSEND"
            loopStuck = loopStuck + 1
        elseif returnData == "FTPCANCEL" then
            chunk = nil
            lastState = UNKNOWN
        elseif loopStuck == 5 then
            lastState = STUCK
            chunk = nil
        end
    end
    socket:write("FTPDATAFIN",fs.size(FileDetails.file))-- maybe add a final md5 hash or something for integrity? hash stored as string, size as number, to differentiate which you've gotten.
    fileToSend:close()
    return true, lastState
end

return FTPCore