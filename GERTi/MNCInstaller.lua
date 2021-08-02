local shell = require("shell")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/GERTiClient.lua /etc/rc.d/GERTiMNC.lua")
shell.execute("wget /lib/MNCAPI.lua")