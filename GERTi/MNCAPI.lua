-- GERT v1.5 Build 2
local computer = require("computer")
local event = require("event")
local filesystem= require("filesystem")
local shell = require("shell")

local function waitWithCancel(timeout, cancelCheck)
	local now = computer.uptime()
	local deadline = now + timeout
	while now < deadline do
		event.pull(deadline - now, "modem_message")
		local response = cancelCheck()
		if response then return response end
		now = computer.uptime()
	end
	return cancelCheck()
end