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
    local matches = {}
	for w in address:gmatch("(%d+%.%d+)") do
		table.insert(matches, w)
	end
	
    if #matches == 0 or #matches > 2 then
        return 0
    end
	
	if #matches == 2 then
		if matches[1] == 0.0 or not address:match("%d+%.%d+:%d+%.%d+") then
			return 0
		end
	end

    for num in address:gmatch("%d+") do
        local conv = math.tointeger(num)
        if not conv or conv > 4095 or conv < 0 then
            return 0
        end
    end

	return #matches
end

---Creates a new GERTAddress from a string
---@param address string
---@return GERTAddress
function GERTAddress:new(address)
	local actual = {}
    local proxy = {}
    setmetatable(actual, { __index = GERTAddress })
    setmetatable(proxy, { 
        __index = function(self, index)
			if index == "address" then
				if actual.external then
					return actual.external .. ":" .. actual.internal
				else
					return actual.internal
				end
			else
				return actual[index]
			end
		end,
        __newindex = function(self, index, value)
            if index == "external" then
				if compliant(value) ~= 1 then
					error("Address is not value")
				end
			
                actual.external = value
            elseif index == "internal" then
				if compliant(value) ~= 1 then
					error("Address is not value")
				end
			
                actual.internal = value
            elseif index == "address" then
				if compliant(value) == 0 then
					error("Address is not valid")
				end
			
                local matches = {}
				for w in address:gmatch("(%d+%.%d+)") do
					table.insert(matches, w)
				end
				
                if matches[2] then
                    actual.internal = matches[2]
					actual.external = matches[1]
                else
                    actual.internal = matches[1]
					actual.external = nil
                end
            end
        end,
		__tostring = function(self)
			return self.address
		end,
		__concat = function(first, second)
			return tostring(first) .. tostring(second)
		end
    })
    
    proxy.address = address
	return proxy
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

setmetatable(GERTAddress, { __newindex = function() end, __call = function(self, ...) return self:new(...) end})
return GERTAddress
