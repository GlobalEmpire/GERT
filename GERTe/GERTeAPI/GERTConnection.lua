local internet = require "internet"
local component = require "component"
local RealTime = require "RealTime"
local computer = require "computer"
local KeyDatabase = require "KeyDatabase"
local GERTPacket = require "GERTPacket"

local data = {}
setmetatable(data, { __index = function(_, index)
    return function(...) component.invoke(component.list("data")(), index, ...) end
end })

---Represents a link to a single relay.
---@class GERTConnection
---@field version string Raw version of the socket link
---@field identity GERTIdentity the identity of the link
local GERTConnection = {}

local VERSION = "\2\0"

---Creates a new connection to the given relay.
---@param ip string
---@param port number
---@param identity GERTIdentity
---@return GERTConnection
function GERTConnection:new(ip, port, identity)
    local o = {}
    setmetatable(o, { __index = GERTConnection })

    o.socket = internet.open(ip, port)

    o.socket:write(VERSION .. identity.addr)
    o.version = o.socket:read(2)

    if o.version == "\0\0" then
        local err = o.socket:read(1)
        o.socket:close()

        if err == '\0' then
            error("Peer does not support this version")
        elseif err == '\1' then
            error("Peer does not recognize our address")
        elseif err == '\2' then
            error("Peer encountered an unknown error")
        end
    end

    local packet = "\0\0" .. RealTime:new():getTimestamp()
    local sig = data.ecdsa(packet, identity.key)
    o.socket:write(packet .. sig)
    o.identity = identity

    return o
end

---Sends a packet over the link.
---@param packet GERTPacket
---@return void
function GERTConnection:write(packet)
    local len str:length()

    packet.source.external = self.identity.address
    local raw = string.char((len >> 8) & 0xFF, len & 0xFF) .. packet.source:toBytes() .. packet.destination:toBytes()
            .. str .. RealTime:new():getTimestamp()
    local sig = data.ecdsa(raw, self.identity.key)
    self.socket:write(raw .. sig)
end

---Reads a single packet from the socket
---@param timeout number Timeout or nil to wait forever
---@return GERTPacket
function GERTConnection:read(timeout)
    if not timeout then
        timeout = math.huge
    end
    self.socket:setTimeout(timeout)

    local now = computer.uptime()

    if not self.length then
        self.raw = self.socket:read(2)
        self.length = (self.raw:byte(1) << 8) | self.raw:byte(2)
    end

    timeout = computer.uptime() - now
    if timeout < 0 then
        error("Read timeout")
    end
    self.socket:setTimeout(timeout)

    if not self.source then
        local raw = self.socket:read(6)
        self.raw = self.raw .. raw
        self.source = GERTAddress:parse(raw)
    end

    timeout = computer.uptime() - now
    if timeout < 0 then
        error("Read timeout")
    end
    self.socket:setTimeout(timeout)

    if not self.destination then
        local raw = self.socket:read(6)
        self.raw = self.raw .. raw
        self.destination = GERTAddress:parse(raw)
    end

    timeout = computer.uptime() - now
    if timeout < 0 then
        error("Read timeout")
    end
    self.socket:setTimeout(timeout)

    if not self.data then
        local raw = self.socket:read(self.length)
        self.raw = self.raw .. raw
        self.data = raw
    end

    timeout = computer.uptime() - now
    if timeout < 0 then
        error("Read timeout")
    end
    self.socket:setTimeout(timeout)

    if not self.timestamp then
        local raw = self.socket:read(8)
        self.raw = self.raw .. raw
        self.timestamp = (self.raw:byte(1) << 24) | (self.raw:byte(2) << 16) | (self.raw:byte(2) << 8) |
                self.raw:byte(2)
    end

    timeout = computer.uptime() - now
    if timeout < 0 then
        error("Read timeout")
    end
    self.socket:setTimeout(timeout)

    local signature = self.socket:read(32)
    local dta = self.data
    local source = self.source
    local destination = self.destination
    local timestamp = RealTime:new(self.timestamp)

    self.length = nil
    self.source = nil
    self.destination = nil
    self.data = nil
    self.timestamp = nil

    if timestamp:within(60) and data.ecdsa(self.raw, KeyDatabase:getKey(self.destination), signature) then
        return GERTPacket:new(source, destination, dta)
    else
        return self:read(timeout - (computer.uptime() - now))
    end
end

setmetatable(GERTConnection, { __newindex = function() end})
return GERTConnection
