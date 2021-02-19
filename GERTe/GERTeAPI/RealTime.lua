---API for retrieving the real world time
---Note: Updates every 5 minutes to avoid decryption errors caused by drift.
---@class RealTime
---@field time number The real unix time
---@field offset number The offset to add to the uptime to calculate unix time
---@field curOffset number The offset at the time of this object's creation
local RealTime = {}

local internet = require "internet"
local computer = require "computer"
local time = {}

local function getRealTime()
    local raw = internet.request("http://worldtimeapi.org/api/timezone/Etc/UTC.txt")()

    for k, v in raw:gmatch("(%a+): (%a+)\n") do
        if k == "unixtime" then
            time.utc = math.tointeger(v)
            time.uptime = computer.uptime()
        end
    end
end

---Creates a new RealTime with the current time or from timestamp
---@param timestamp string
---@return RealTime
function RealTime:new(timestamp)
    local o = {}
    setmetatable(o, { __index = RealTime })

    if timestamp then
        o.time = (timestamp:byte(1) << 24) | (timestamp:byte(2) << 16) | (timestamp:byte(3) << 8) | timestamp:byte(4)
        o.curOffset = RealTime.offset
    else
        o:getTime()
    end
end

---Gets the real unix time
---@return number
function RealTime:getTime()
    if computer.uptime() - time.uptime > 300 then
        getRealTime()
    end

    self.time = time.utc + (computer.uptime() - time.uptime)
    self.curOffset = time.utc - time.uptime
    RealTime.offset = self.curOffset
end

---Gets the raw 64-bit unix timestamp
---@return string
function RealTime:getTimestamp()
    return string.char((self.time >> 24) & 0xFF, (self.time >> 16) & 0xFF, (self.time >> 8) & 0xFF, self.time & 0xFF)
end

---Returns if the object represents a time within the given interval
---@param seconds number
---@return boolean
function RealTIme:within(seconds)
    return computer.uptime() + self.curOffset > self.time + seconds
end

getRealTime()

local ref = {}
setmetatable(ref, { __index = RealTime, __newindex = function() end })
return ref
