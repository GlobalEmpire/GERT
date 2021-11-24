local shell = require("shell")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/GERTiMNC.lua /etc/rc.d/GERTiMNC.lua")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/MNCAPI.lua /lib/GERTiClient.lua")
shell.execute("wget https://raw.githubusercontent.com/GlobalEmpire/GERT/master/GERTi/Modules/DNS.lua /etc/rc.d/DNS.lua")
