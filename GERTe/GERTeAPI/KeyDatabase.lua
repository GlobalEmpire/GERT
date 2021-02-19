---Manages the public key and address relations for other clients.
---@class KeyDatabase
local KeyDatabase = {}

---Retrieves the key for the given address or nil
---@param address GERTAddress
---@return userdata
function KeyDatabase:getKey(address)

end

setmetatable(KeyDatabase, { __newindex = function(self, index, value) end})
return KeyDatabase
