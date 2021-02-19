---A packet of data with a source and a destination.
---@class GERTPacket
---@field source GERTAddress
---@field destination GERTAddress
---@field data string
local GERTPacket = {}

---Creates a new packet with a source, destination, and some data
---@param source GERTAddress
---@param destination GERTAddress
---@param data string
---@return GERTPacket
function GERTPacket:new(source, destination, data)
    local o = {}
    setmetatable(o, { __index = GERTPacket })

    o.source = source
    o.destination = destination
    o.data = data
end

setmetatable(GERTPacket, { __newindex = function() end })
return GERTPacket
