local shell = require("shell")
local fs = require("filesystem")
local autorunList = {"/autorun","/autorun.lua","/.autorun","/.autorun.lua"}
local autorunDirectory = "/autorun"

local fullAutoRun = ""

shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/GERTiMNC.lua /etc/rc.d/GERTiMNC.lua -f")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/MNCAPI.lua /lib/GERTiClient.lua -f")

if not fs.isDirectory("/usr/lib") then
    fs.makeDirectory("/usr/lib")
end

shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/GERTUpdateServer.lua /usr/lib/GERTUpdateServer.lua -f")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/DNS.lua /etc/rc.d/DNS.lua -f")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/RecommendedConfig-GUSs.cfg /etc/GERTUpdateServer.cfg")

for k,v in ipairs(autorunList) do
    if fs.exists(v) then
        autorunDirectory = v
    end
end

local tempfile = io.open(autorunDirectory,"r")

if tempfile then 
    fullAutoRun = tempfile:read("*a")
    tempfile:close()
end

if not string.find(fullAutoRun,"require('GERTUpdateServer')") then
    local tempfile = io.open("/.autorun","a")
    tempfile:write("\nrequire('GERTUpdateServer')")
    tempfile:close()
end

shell.execute("rc GERTiMNC enable")
shell.execute("rc DNS enable")
