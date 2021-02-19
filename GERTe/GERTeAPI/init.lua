-- GERT v2.0 - Release Candidate 1
-- Copyright 2021 Tyler Kuhn All Rights Reserved

local component = require "component"

if not component then
    apiRequired("Component")
end

---GERTe API for creating and utilizing GERTe links.
---@class GERTeAPI
---@field VERSION string Protocol version
---@field GERTConnection table GERTConnection class
---@field GERTAddress table GERTAddress class
---@field GERTIdentity table GERTIdentity class
local api = {
    VERSION = "2.0.0",
    GERTConnection = require "GERTConnection",
    GERTAddress = require "GERTAddress",
    GERTIdentity = require "GERTIdentity"
}

if component.invoke(component.list("internet")(), "isTcpEnabled") == false then
    error("TCP Sockets are not permitted. If this is a server contact the administrator. If this is single player please modify the configuration file.")
end

local function apiRequired(name)
    error(name .. " API from OpenComputers required. Either this isn't an OpenComputer's computer, or this API is hidden")
end

setmetatable(api, { __newindex = function(self, index, value) end})
return api
