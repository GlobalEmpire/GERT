-- FTP Core - Separation Update (Release 1 Beta 1) |R1B1
local fs = require("filesystem")
local GERTi = require("GERTiClient")
local event = require("event")

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

--Op Codes
local ALLGOOD = 0
local DOWNLOADED = 1
local ALREADYINSTALLED = 10

local FTPInternal = {}
local FTPCore = {}

FTPInternal.CreateValidSocket = function (FileDetails)
    local socket = GERTi.openSocket(FileDetails.address,FileDetails.port)
    local serverPresence = false
    if socket then serverPresence = event.pullFiltered(5,function (eventName,oAdd,CID) return eventName=="GERTConnectionID" and oAdd==FileDetails.address and CID==FileDetails.port end) end
    if not serverPresence then
        socket:close()
        return false, NOSOCKET
    end
    return true, socket
end




FTPInternal.CheckData = function (FileDetails,StepComplete,socket)
    if not StepComplete then
        return StepComplete, socket
    end
    --file,address,port,user,auth
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

FTPInternal.DownloadFile = function () --Provide file as relative path on server, destination as where it goes, address of the server.
    --file,destination,address,port,user,auth
    local remoteData, code = FTPCore.CheckData(file,address,port,user,auth)
    if not remoteData then
        return false, code
    end
    local remoteSize = remoteData[2]
    if remoteSize == 0 then
        return false, NOREMOTEFILE
    end
    local filename = fs.name(file)
    local storageDrive = fs.get(destination .. "/" .. filename)
    local remainingSpace = (storageDrive.spaceTotal()-storageDrive.spaceUsed())
    if remoteSize > remainingSpace - 1000 then
        return false, NOSPACE -- insufficient space for download
    end
    local destfile = io.open(destination .. "/" .. filename, "wb")
    local done = false
    socket:write("FTPREADYTORECEIVE",file,user,auth)
    repeat
        local response = event.pullFiltered(5, function (event, iAdd, dAdd, CID) if (iAdd == address or dAdd == address) and (dAdd == port or CID == port) then if event == "GERTConnectionClose" or event == "GERTData" then return true end end return false end)
        if not response then
            socket:close()
            return false, TIMEOUT
        elseif response == "GERTConnectionClose" then
            if #socket:read("-k") > 1 then
                destfile:write(socket:read()[1])
            end
            done = true
        else
            destfile:write(socket:read()[1])
            socket:write("FTPREADYTOCONTINUERECEIVE")
        end
    until done
    socket:close()
    destfile:close()
    if fs.size(destination .. "/" .. filename) == remoteSize then
        return true
    else
        return false, INTERRUPTED -- connection interrupted
    end
end


FTPInternal.SendFile = function (file,directory,address,port,user,auth) -- returns true if successful, false if timeout or invalid modulename
    --Do Auth Processing in a separate function and append it to destination. if not user then user = "public"
    local fileLocation = directory .. "/" .. file

    local fileToSend = io.open(fileLocation, "rb")

    local chunk = fileToSend:read(8000)
    while chunk ~= nil and chunk ~= "" do
        updateSockets[originAddress]:write(chunk)
        local success = event.pull(10, "GERTData", originAddress, updatePort)
        if not success then
            updateSockets[originAddress]:close()
            fileToSend:close()
            return false, TIMEOUT
        end
        chunk = fileToSend:read(8000)
    end
    fileToSend:close()
    os.sleep(0.5)
    updateSockets[originAddress]:close()
    updateSockets[originAddress] = nil
    return true, ALLGOOD
end

FTPCore.DownloadFile = function (FileDetails)
    --[[parameters = {"file","destination","address","port","user","auth"}
    file: obligatory; is the relative File Path as is stored in whatever is sending the file (taking into account scope)
    destination: obligatory; where the file will end up on the downloading device. NOTE! The destination must be a valid file location, or the program will error. There is no default!
    address: obligatory; this is the connector's address
    ]]
    local StepComplete,socket = FTPInternal.CreateValidSocket(FileDetails)
    local StepComplete,result = FTPInternal.CheckData(FileDetails,StepComplete,socket)
    local StepComplete,result = FTPInternal.DownloadFile(FileDetails,socket,StepComplete,result)
end


return FTPCore