---API for retrieving the real world time
---Note: Updates every 5 minutes to avoid decryption errors caused by drift.
---@class RealTime
---@field time number The real unix time
local RealTime = {}

local internet = require "internet"
local computer = require "computer"
local filesystem = require "filesystem"

local function getRealTime()
    filesystem.open("/tmp/gerte.time", "w"):close()
	return filesystem.lastModified("/tmp/gerte.time")
end

---Creates a new RealTime with the current time or from timestamp
---@param timestamp string
---@return RealTime
function RealTime:new(timestamp)
    local o = {}
    setmetatable(o, { __index = RealTime })

    if timestamp then
        o.time = string.unpack(">I8", timestamp)
    else
        o:getTime()
    end
	
	return o
end

---Gets the real unix time
---@return number
function RealTime:getTime()
    self.time = math.floor(getRealTime() / 1000)
end

---Gets the raw 64-bit unix timestamp
---@return string
function RealTime:getTimestamp()
	-- return string.char(self.time & 0xFF, (self.time >> 8) & 0xFF, (self.time >> 16) & 0xFF, (self.time >> 24) & 0xFF)
	return string.pack(">I8", self.time)
end

---Returns if the object represents a time within the given interval
---@param seconds number
---@return boolean
function RealTime:within(seconds)
    return getRealTime() > self.time + seconds
end

setmetatable(RealTime, {__newindex = function() end, __call = function(self, ...) return self:new(...) end})
return RealTime
