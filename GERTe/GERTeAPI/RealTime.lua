---API for retrieving the real world time
---Note: Updates every 5 minutes to avoid decryption errors caused by drift.
---@class RealTime
---@field time number The real unix time
local RealTime = {}

local internet = require "internet"
local computer = require "computer"

local function getRealTime()
    filesystem.open("/tmp/gerte.time", "w"):close()
	return filesystem.lastModified("/tmp/gerte.time") / 1000
end

---Creates a new RealTime with the current time or from timestamp
---@param timestamp string
---@return RealTime
function RealTime:new(timestamp)
    local o = {}
    setmetatable(o, { __index = RealTime })

    if timestamp then
        o.time = (timestamp:byte(1) << 24) | (timestamp:byte(2) << 16) | (timestamp:byte(3) << 8) | timestamp:byte(4)
    else
        o:getTime()
    end
end

---Gets the real unix time
---@return number
function RealTime:getTime()
    self.time = getRealTime()
end

---Gets the raw 64-bit unix timestamp
---@return string
function RealTime:getTimestamp()
    return string.char((self.time >> 24) & 0xFF, (self.time >> 16) & 0xFF, (self.time >> 8) & 0xFF, self.time & 0xFF)
end

---Returns if the object represents a time within the given interval
---@param seconds number
---@return boolean
function RealTime:within(seconds)
    return getRealTime() > self.time + seconds
end

local ref = {}
setmetatable(ref, { __index = RealTime, __newindex = function() end })
return ref
