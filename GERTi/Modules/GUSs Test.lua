os.sleep(5)
local fs = require("filesystem")
fs.remove("/etc/GERTUpdateServer.cfg")
package.loaded.GUS = nil
local GUS = require("GERTUpdateServer")
local srl = require("serialization")
local event = require("event")

local function iterToTable (iterable)
    local b = {}
    for a in iterable do
        table.insert(b,a)
    end
    return b
end

----
print("SyncNewModule Test Begin")
print(srl.serialize(iterToTable(fs.list("/usr/lib"))))
print(srl.serialize(iterToTable(fs.list("/modules"))))
local SNMT1 = {
table.pack(GUS.SyncNewModule("DNS.lua","/etc/rc.d/DNS.lua")),
table.pack(GUS.SyncNewModule("DNS.lua","/etc/rc.d/DNS.lua")),
table.pack(GUS.SyncNewModule("GERTUpdateServer.lua")),
table.pack(GUS.SyncNewModule("GERTUpdateServer.lua")),
table.pack(GUS.SyncNewModule("MNCAPI.lua","/usr/lib/GERTiClient.lua")),
table.pack(GUS.SyncNewModule("GERTiClient.lua","/modules/GERTiClient.lua"))}
if srl.serialize(SNMT1) == srl.serialize({{true,n=1},{false,4,n=2},{false,4,n=2},{false,4,n=2},{false,4,n=2},{false,4,n=2}}) then
    print("Test Succeeded")
else
    print("Test Failed")
end
----

print("RemoveModule Test Begin")
local RMT = {
table.pack(GUS.RemoveModule("DNS.lua")),
table.pack(GUS.RemoveModule("DNS.lua")),
table.pack(GUS.RemoveModule("GERTUpdateServer.lua")),
table.pack(GUS.RemoveModule("GERTUpdateServer.lua")),
table.pack(GUS.RemoveModule("MNCAPI.lua")),
table.pack(GUS.RemoveModule("GERTiClient.lua"))}
if srl.serialize(RMT) == srl.serialize({{true,n=1},{false,n=1},{false,n=1},{false,n=1},{false,n=1},{false,n=1}}) then
    print("Test Succeeded")
else
    print("Test Failed")
end
----

print("SyncNewModule Secondary Test Begin")
print(srl.serialize(iterToTable(fs.list("/usr/lib"))))
print(srl.serialize(iterToTable(fs.list("/modules"))))
local SNMT2 = {
    table.pack(GUS.SyncNewModule("DNS.lua","/etc/rc.d/DNS.lua")),
    table.pack(GUS.SyncNewModule("DNS.lua","/etc/rc.d/DNS.lua")),
    table.pack(GUS.SyncNewModule("GERTUpdateServer.lua")),
    table.pack(GUS.SyncNewModule("GERTUpdateServer.lua")),
    table.pack(GUS.SyncNewModule("MNCAPI.lua","/usr/lib/GERTiClient.lua")),
    table.pack(GUS.SyncNewModule("GERTiClient.lua","/modules/GERTiClient.lua"))}
if srl.serialize(SNMT2) == srl.serialize({{true,n=1},{false,4,n=2},{false,4,n=2},{false,4,n=2},{false,4,n=2},{false,4,n=2}}) then
    print("Test Succeeded")
else
    print("Test Failed")
end
----

print("CheckLatest Test Begin")
print(srl.serialize(iterToTable(fs.list("/usr/lib"))))
print(srl.serialize(iterToTable(fs.list("/modules"))))
local CLT = {
    table.pack(GUS.CheckLatest("DNS.lua")),
    table.pack(GUS.CheckLatest("DNS.lua")),
    table.pack(GUS.CheckLatest("GERTUpdateServer.lua")),
    table.pack(GUS.CheckLatest("GERTUpdateServer.lua")),
    table.pack(GUS.CheckLatest("MNCAPI.lua")),
    table.pack(GUS.CheckLatest("MNCAPI.lua")),
    table.pack(GUS.CheckLatest("GERTiClient.lua")),
    table.pack(GUS.CheckLatest("GERTiClient.lua"))}
print(srl.serialize(CLT))
----

print("Listener Handler Check")
print(srl.serialize(GUS.listeners))
print(srl.serialize(GUS.StartHandlers()))
print(srl.serialize(GUS.StartHandlers()))
print(srl.serialize(GUS.StopHandlers()))
print(srl.serialize(GUS.StopHandlers()))
print(srl.serialize(GUS.StartHandlers()))
print(srl.serialize(GUS.StartHandlers()))

