# GERT Update Suite
## Configuration files for both programs. They are created on each run if they do not exist.
 **For GERTUpdateServer.lua:** Located at `"/etc/GERTUpdater.cfg"`<br>
  The first line is a serialized table of config options for the program. For now, it will always be empty because there are no config options. This is for future options.<br>
 All remaining lines designate paths for modules. Each module will occupy a pair of two lines. The first line will contain the name of the module and is used to access the module on GitHub. For example, GERTiClient.lua, or Modules/DNS.lua. The second line is the file path on the local machine where the module will be stored, and this path is assumed to be absolute: *ensure that it leads with a / and ends with the extension of the module.*<br>
 Each line contains a module path, and the filename resolved using fs.name() is considered the module name.
 >As it stands, it is not possible to add or remove new modules through the API, they must be added manually into this file. This is a future feature, but one considered less integral to the program's function, so I have delayed adding it.

 **For GERTCUpdater.lua:** Located at `"/usr/lib/GERTUpdater.cfg"`<br>
 The first line is a serialized table of config options for the program. As of right now, the only option is "AutoUpdate", which is false by default. If true, the program will install updates as soon as it receives the event from the concerned program that it is ready to install its update. More details on this behaviour later.<br>
 The rest is identical to `GERTUpdateServer.lua`, each pair of lines is a name and path.
 >It **is** possible to add and remove modules through the API by calling `InstallNewModule()` and `RemoveModule()`.

 **Cache File:** This is used by `GERTCUpdater.lua` and `SafeUpdater.lua` to safe updates between power loss. It is located under `"/.SafeUpdateCache.srl"` at the root directory.<br>
 **Cache Folder:** This stores the updates for programs, it is  `"/.moduleCache/"`

 **Other Technical Information:**

- The Port used for the connection is `941`.<br>
- The client program is setup to contact the server through the `"GERTModules"` Address. If DNS is not setup, please modify line 14 in `GERTCUpdater.lua`. <br>
- `GERTCUpdater` is capable of running without the `SafeUpdater` program, however it will then only update programs when the user requests it, or if `autoInstall` is true, the target program responds properly, and the computer does not reboot between the time the update is downloaded and the time when the target program safes itself. <br>

# API functions: 
## **GERTUpdateServer.lua** <br>
 ### **`GMU.CheckLatest(moduleName)`**:<br>
 Accepts one variable: the module's name.<br>
 Reads the header of the provided module's cache if it exists, then downloads and reads the header of the remote file. If the remote file has a different version header, it will be downloaded and **will replace** the current cached file.
 
 Success is determined by whether or not there is a file ready to be sent to clients. <br>
 If Successful, it will return 3 parameters: <br>
>  `true, $code, $versionHeader` <br>
- `$code` is the operation code:
  - `0`: The requested module was already up to date on the cache.<br>
  - `-1`: The file was downloaded from remote and replaced the cached version. <br>
  - `-2`: The function could not establish a connection to remote, but a cached file is present. <br>

- `$versionHeader` is the version of the file that is currently on drive.<br>
 
 No success means that there is no file on-server that can be used.<br>
 If not Successful, it will return 2 parameters: <br>
> `false, $code`<br>
- `$code` is an error code:
  - `1`: A connection could not be established to Remote.<br>
  - `2`: This module's path is not present in the configuration file.<br>
  - `3`: There is insufficient space on the server to download the updated file. The local version either does not exist, or is outdated relative to Remote. This does support multiple drives, but will not move the cached file to a different drive if its drive is full.<br>

### **`GMU.StartHandlers():`**
 Starts all event handlers -- *is called when GERTUpdateServer.lua is `require`d. I can change this.* <br>
 
### **`GMU.StopHandlers():`**
 Stops all event handlers <br>

 **`GMU.listeners`** is a table that contains the IDs of all the event handlers, under the keys `GERTUpdateSocketOpenerID`, `GERTUpdateSocketCloserID`, `GERTUpdateSocketHandlerID` <br>

## **GERTCUpdater.lua** <br>
### **`GCU.GetLocalVersion(path):`**
This function reads and returns the version header (the first line) of the file at the provided path. If the file does not exist, or the path points towards a directory, it will return an empty string.<br>
Accepts one variable: a `path`.
> This accepts a `path` and not a `moduleName` so that advanced users can use it to check the version of other files, such as cached files.

### **`GCU.GetRemoteVersion(moduleName,socket):`**
This function requests the `versionHeader` of `moduleName` from the update server.<br>
Accepts two variables: `moduleName` and `socket`.
- `moduleName` is the name of the module, which will frequently begin with Modules/ if it is not part of a base GERTi installation (f.e. Modules/DNS.lua).
- `socket` can be provided to cause the function to piggyback off of an existing socket. Useful for advanced users, who might have multiple program update servers. For your own safety, it is recommended to pcall() this function if a custom socket is used as the function assumes a valid socket and cannot handle an invalid socket object. **This feature is not yet standard in all functions, this will change eventually.**


If the function succeeds, it returns 4 parameters:
> `true`, `$state`, `$size`, `$version`
- `$State` is any code that the server sent alongside the information. These are the operation codes returned by `GMU.CheckLatest()`.
- `$size` is the size in bytes of the file on the server. 0 if the file could not be located for any reason.
- `$version` is the version header of the file on the server.

If the function fails, it returns 2 parameters:
> `false`, `$code`
- `$code` is an error code that can be of the following values:
  - For positive values, refer to `GMU.CheckLatest()` under the failure state.
  - For zero/negative values:
    - `0`: `moduleName` was either not provided or invalid
    - `-1`: A socket could not be established (only possible if socket not provided)
    - `-2`: Timed-out: The server did not respond fast enough, if at all.
    - `-3`: The server returned unexpected information through the socket. This information is passed as another variable after `$code`.

### **`GCU.CheckForUpdate(moduleName):`**
Returns information about the module/modules provided.<br>
Accepts one variable: `moduleName`.
- `moduleName` is the name of the module, which will frequently begin with Modules/ if it is not part of a base GERTi installation (f.e. Modules/DNS.lua).
- Alternatively, you can pass a `table` in the format of `table.[moduleName]=modulePath` and the function will evaluate each module and its provided path.
- **Passing nothing to the function will make the function evaluate every module set in the configuration file.**

If the function succeeds, it will return: 
> `true`, `$infoTable`

The content of `$infoTable` depends on whether it was passed a `moduleName` or a `table` of `(moduleName,modulePath)=(key,value)` pairs, henceforth referred to as a `moduleTable`.
- Case 1: `moduleName`
  - It will return an ordered table of 6 values, containing `localVersion`, `localSize`, `remoteVersion`, `remoteSize`, `statusCode`, `success` in that order.
    - `localVersion`: The version of the currently installed module.
    - `localSize`: The size of the currently installed module in bytes.
    - `remoteVersion`: The version string of the module currently available on the server.
    - `remoteSize`: The size of the module available on the server in bytes. 0 if not present on server
    - `statusCode`,`success`: This function wraps `GCU.GetRemoteVersion()`, the result of which is placed into `success`,`statusCode`,`remoteSize`,`remoteVersion` in that order. Use the value of `success` to determine the meaning of `statusCode`
- Case 2: A `moduleTable` or nothing at all was passed:
  - It will return a table indexed by moduleName, each containing an instance of a table from Case 1.


If the function fails, it will return:
> `false`, `1`

*The only possible non-crashing error this function can encounter is no response from address which returns 1*

### **`GCU.DownloadUpdate(moduleName,infoTable,InstallWhenReady):`**
This function downloads to cache the updated version of modules if they are out of date. It also registers an entry in a safe file that is read every boot up, so that the file is updated on next boot up.<br>
Accepts 3 optional variables: `moduleName`, `infoTable`, `InstallWhenReady`. <br>
- Provide `moduleName` or a `moduleTable` to have the function check only the modules provided. Otherwise, will check all modules in the configuration file.
- If you have already obtained `infoTable` by running `GCU.CheckForUpdate()` you can provide it here to stop the function requesting it again. If `moduleTable` is passed, any `moduleNames` that do not have an entry in `infoTable` will be generated automatically.
- If `InstallWhenReady` is true, the function will push an event when each module is downloaded with the parameters `"UpdateAvailable"`,`<moduleName>`. This will allow modules that run as a service to gracefully shutdown and prepare themselves for an update.<br> 
When the module has finished shutting down, it must push an event with the parameters `"InstallReady"`, `<moduleName>` to alert the updater that it is ready to be installed.<br> 
When the updater has finished installing the update, it will push an event with `"ModuleInstall"`,`<moduleName>` and then the results of `GCU.InstallUpdate()` as parameters.<br> 
`"ModuleInstall"` allows programs to register an event listener before pushing `"InstallReady"` to automatically start back up when the update has concluded.<br>
- *All modules are expected to be able to respond to the `"UpdateAvailable"`, even if they do not need to gracefully shutdown or restart after the update. **At minimum, a module must include a simple event listener that pushes `"InstallReady"`,`<moduleName>` when the relevant `"UpdateAvailable"` is pushed.*** 


### **`GCU.InstallUpdate(moduleName):`**
Accepts one variable:



# Examples:
## Requesting modules
Assume the following:

- The configuration file for the update server looks like this:

        {}
        /usr/lib/DNS.lua
        /usr/lib/DNSServer.lua
        /usr/lib/GERTUpdateServer.lua
        /usr/lib/GERTCUpdater.lua
        /usr/lib/GERTiClient.lua

- The configuration file for a client looks like this:

        {AutoUpdate=false}
        /usr/lib/GERTCUpdater.lua
        /usr/lib/GERTiClient.lua

If the user wants to manually check for an update to the `GERTiClient.lua` module and install it as soon as possible from the server if it exists, they would have the following code in a file:

    local GCU = require("GERTCUpdater")
    local Success, StateTable, Additional = GCU.DownloadUpdate("GERTiClient.lua",nil,true)

As for understanding the data:<br> 
`Success` is a simple boolean determining whether or not the operation succeeded: if it is true, the update was successfully downloaded and queued for installation on reboot (and queued for installation if `InstallImmediately` is true).<br>
`StateTable` is an `infoTable`, this can be searched for relevant parameters, such as whether or not the server was able to establish a connection to remote.<br>
`Additional` is any additional variables that get passed, for any reason. Should be nil Success is true.

----

Now, if the user wants to do the same thing as before, but with two modules, say, `GERTiClient.lua` and `GERTCUpdater.lua`, they would do the following:

    local GCU = require("GERTCUpdater")
    local moduleTable = {}
    moduleTable["GERTiClient.lua"] = "/usr/lib/GERTiClient.lua"
    moduleTable["GERTCUpdater.lua"] = "/usr/lib/GERTCUpdater.lua"
    local Success, stateTable = GCU.DownloadUpdate(moduleTable,nil,true)

The results are much same, except they are indexed by module name in `stateTable`. For this specific use case, due to the fact that the modules we requested are identical to the modules that exist in the configuration file, we would obtain the same result by calling `GCU.DownloadUpdate(nil,nil,true)`.
If we set the `AutoUpdate` configuration setting to `true`, then we would no longer need to specify `true`, and would be able to simply call `GCU.DownloadUpdate()`.

# Suggested Configuration Defaults

Client-side:

    ​{AutoUpdate=false}
    ​/usr/lib/GERTCUpdater.lua
    ​/usr/lib/GERTiClient.lua
    /etc/rc.d/SafeUpdater.lua

Server-side:
    
    {}
    /usr/lib/DNS.lua
    /usr/lib/GERTUpdateServer.lua
    /usr/lib/GERTCUpdater.lua
    /usr/lib/GERTiClient.lua
    /usr/lib/SafeUpdater.lua