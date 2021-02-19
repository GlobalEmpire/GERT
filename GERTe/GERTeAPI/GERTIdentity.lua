---Represents an identity of a client.
---@class GERTIdentity
---@field address string The address of this identity
---@field key userdata The private key of this identity
local GERTIdentity = {}

local function parseAddr(addr)
    local externalUpper, externalLower = string.match(addr, "(%d+)%.(%d+)")
    externalUpper = tonumber(externalUpper)
    externalLower = tonumber(externalLower)
    local chars = {
        string.char(externalUpper >> 4),
        string.char(((externalUpper & 0x0F) << 4) | (externalLower >> 8)),
        string.char(externalLower & 0xFF),
    }
    return table.concat(chars, "")
end

---Creates a new identity from an address and private key
---@param address GERTAddress
---@param key userdata
---@return GERTIdentity
function GERTIdentity:new(address, key)
    if key.isPublic() then
        error("GERT Identities must use private keys")
    end

    local o = {}
    setmetatable(o, { __index = GERTIdentity })

    o.address = address.external or address.internal
    o.addr = parseAddr(o.address)
    o.key = key

    return o
end

setmetatable(GERTIdentity, { __newindex = function() end})
return GERTIdentity
