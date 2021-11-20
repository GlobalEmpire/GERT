local event = require("event")
local fs = require("filesystem")
local shell = require("shell")
local srl = require("serialization")

local args, opts = shell.parse(...)

local GERTUpdaterAPI = {}








event.listen("InstallReady",InstallEventHandler)

return GERTUpdaterAPI