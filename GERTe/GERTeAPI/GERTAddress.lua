---Represents a GERT Address, both internal, and complete.
---@class GERTAddress
---@field internal string The internal address
---@field external string The external address (empty for internal only)
---@field address string The entire address
local GERTAddress = {}

local function parseAddr(addr)
    local addrPart = "%d?%d?%d?%d"
    local addrSegment = addrPart .. "%." .. addrPart
    local externalUpper = string.match(addr, "(" .. addrPart .. ")%." .. addrPart .. ":" .. addrSegment)
    local externalLower = string.match(addr, addrPart .. "%.(" .. addrPart .. "):" .. addrSegment)
    local internalUpper = string.match(addr, addrSegment .. ":(" .. addrPart .. ")%." .. addrPart)
    local internalLower = string.match(addr, addrSegment .. ":" .. addrPart .. "%.(" .. addrPart .. ")")
    externalUpper = tonumber(externalUpper)
    externalLower = tonumber(externalLower)
    internalUpper = tonumber(internalUpper)
    internalLower = tonumber(internalLower)
    local chars = {
        string.char(externalUpper >> 4),
        string.char(((externalUpper & 0x0F) << 4) | (externalLower >> 8)),
        string.char(externalLower & 0xFF),
        string.char(internalUpper >> 4),
        string.char(((internalUpper & 0x0F) << 4) | (internalLower >> 8)),
        string.char(internalLower & 0xFF),
    }
    return table.concat(chars, "")
end

local function unparseAddr(addr)
    local chars = {
        addr:byte(1),
        addr:byte(2),
        addr:byte(3),
        addr:byte(4),
        addr:byte(5),
        addr:byte(6)
    }
    parts = {
        tostring((chars[1] << 4) | (chars[2] >> 4)),
        tostring(((chars[2] & 0x0F) << 8) | chars[3]),
        tostring((chars[4] << 4) | (chars[5] >> 4)),
        tostring(((chars[5] & 0x0F) << 8) | chars[6]),
    }
    return parts[1] .. "." .. parts[2] .. ":" .. parts[3] .. "." .. parts[4]
end

local function compliant(address)
    local matches = { address:match("(%d+%.%d+:)?(%d+%.%d+)") }
    if not matches[1] then
        return false
    end

    matches[1] = matches[1]:gsub(":", "")

    if matches[2] and matches[1] == "0.0" then
        return false
    end

    local nums = 0
    for num in address:gmatch("%d+") do
        nums = nums + 1
        if nums > 4 then
            return false
        end

        local conv = math.tointeger(num)
        if not conv or conv > 4095 or conv < 0 then
            return false
        end
    end

    return true
end

---Creates a new GERTAddress from a string
---@param address string
---@return GERTAddress
function GERTAddress:new(address)
    compliant(address)

    local o = { actual = {} }
    setmetatable(o.actual, { __index = GERTAddress })
    setmetatable(o, { 
        __index = o.actual,
        __newindex = function(self, index, value)
            if index == "external" then
                self.actual.external = value
                self.actual.address = value .. ":" .. self.actual.internal
            elseif index == "internal" then
                self.actual.internal = value

                if self.actual.external then
                    self.actual.address = self.actual.external .. ":" .. value
                else
                    self.actual.address = value
                end
            elseif index == "address" then
                self.actual.address = value

                local matches = ({ value:match("(%d+%.%d+:)?(%d+%.%d+)") })
                if matches[2] then
                    self.actual.internal = matches[2]
                else
                    self.actual.internal = matches[1]
                end

                self.actual.external = value:match("(%d+%.%d+):%d+%.%d+")
            else
                self[index] = value
            end
        end
    })
    
    o.address = address
    local matches = ({ address:match("(%d+%.%d+:)?(%d+%.%d+)") })
    if matches[2] then
        o.internal = matches[2]
    else
        o.internal = matches[1]
    end

    o.external = address:match("(%d+%.%d+):%d+%.%d+")
end

---Parses a GERTc Address from 6 bytes
---@param bytes string
---@return GERTAddress
function GERTAddress:parse(bytes)
    return GERTAddress:new(unparseAddr(bytes))
end

---Converts an address to raw bytes
---@return string
function GERTAddress:toBytes()
    return parseAddr(self.address)
end

setmetatable(GERTAddress, { __newindex = function() end})
return GERTAddress
