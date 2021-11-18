local event = require("event")
local MNCAPI


function start()
    if package.loaded()["GERTiClient"] then
        MNCAPI = require("GERTiClient")
        if MNCAPI.getEdition() ~= "MNCAPI" then
            return
        end

    end
end