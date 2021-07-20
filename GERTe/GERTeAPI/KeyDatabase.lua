local component = require "component"

local data = component.proxy(component.list("data")())

---Manages the public key and address relations for other clients.
---@class KeyDatabase
local KeyDatabase = {}

local keys = {}

local function unparseAddr(addr)
    local chars = {
        addr:byte(1),
        addr:byte(2),
        addr:byte(3)
    }
    parts = {
        tostring((chars[1] << 4) | (chars[2] >> 4)),
        tostring(((chars[2] & 0x0F) << 8) | chars[3])
    }
    return parts[1] .. "." .. parts[2]
end

---Retrieves the key for the given address or nil
---@param address GERTAddress
---@return userdata or nil
function KeyDatabase:getKey(address)
	local addr = address.external or address.internal
	return keys[addr]
end

---Loads the key database from resolutions.geds
function KeyDatabase:load()
	local file = io.open("resolutions.geds", "rb")
	
	while true do
		local addrRaw = file:read(3)
		if not addrRaw then
			break
		end
		
		local address = unparseAddr(addrRaw)
		
		local lenRaw = file:read(2)
		if not lenRaw then
			break
		end
		
		local length = string.unpack(">I2", lenRaw)
		
		local keyRaw = file:read(length)
		if not keyRaw then
			break
		end
		
		local key = data.deserializeKey(keyRaw, "ec-public")
		
		keys[address] = key
	end
	
	file:close()
end

setmetatable(KeyDatabase, { __newindex = function() end, __call = function(self) return self:load() end})
return KeyDatabase
