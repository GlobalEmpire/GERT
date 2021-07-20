-- GERT v2.0 - Release Candidate 1
-- Copyright 2021 Tyler Kuhn All Rights Reserved

local cwd = ...
local component = require "component"

if not component then
    error("Component API from OpenComputers required. Either this isn't an OpenComputer's computer, or this API is hidden")
end

local internet = component.list("internet")()
if not internet then
	error("No internet card detected")
end

local data = component.list("data")()
if not data then
	error("No data card detected")
end

if component.invoke(component.list("internet")(), "isTcpEnabled") == false then
    error("TCP Sockets are not permitted. If this is a server contact the administrator. If this is single player please modify the configuration file.")
end

---GERTe API for creating and utilizing GERTe links.
---@class GERTeAPI
---@field VERSION string Protocol version
---@field GERTConnection table GERTConnection class
---@field GERTAddress table GERTAddress class
---@field GERTIdentity table GERTIdentity class

local path = package.path
package.path = package.path .. ";/" .. cwd .. "/?.lua"

local api = {
    VERSION = "2.0.0",
    Connection = require "GERTConnection",
    Address = require "GERTAddress",
    Identity = require "GERTIdentity",
	Packet = require "GERTPacket",
	KeyDatabase = require "KeyDatabase"
}

package.path = path

setmetatable(api, { __newindex = function(self, index, value) end})
return api
