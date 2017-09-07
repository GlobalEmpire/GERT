#define WIN32_LEAN_AND_MEAN //Windows Optimization flag preventing loading of certain libraries
#ifdef _WIN32 //If we're compiled in Windows
#include <Windows.h> //Load Windows? Wait? Windows wasn't loaded? How'd we run? *Mind Blown*
#include <filesystem> //Load Windows filesystem API
typedef HMODULE lib; //Define lib as Windows HMODULE for dynamic library
#else //If we're compiled in *nix
#include <dlfcn.h> //Load Unix Dynamic Library... library
#include <experimental/filesystem> //Load experimental C++ Standard Filesystem API
typedef void* lib; //Define lib as Unix void pointer for dynamic library
#endif
#include <map> //Load map type for databases
#include "netty.h" //Load netty header for ... Reason? Notify me if you determine what is required
#include "libLoad.h"
#include "logging.h"
typedef unsigned char UCHAR;
using namespace std; //Default to using STD namespace
using namespace experimental::filesystem::v1; //Default to using this really long namespace

map<UCHAR, Version*> registered; //Create protocol library database

enum libErrors { //Create list of library errors
	NO_ERR, //We're good!
	UNKNOWN, //Unknown error
	EMPTY //No libraries detected
};

void* getValue(void* handle, string value) { //Retrieve library value
#ifdef _WIN32 //If compiled for Windows
	return GetProcAddress(*(lib*)handle, value.c_str()); //Retrieve using Windows API
#else //If not compiled for Windows
	void * test = dlsym(*(lib*)handle, value.c_str()); //Get value using Unix API
	char * err = dlerror(); //Detect potential error
	if (err != nullptr) { //If error isn't empty
		error(err); //Let's error it!
		return nullptr; //Return null
	}
	return test; //Retrieve using C++ standard API
#endif
}

lib loadLib(path libPath) { //Load protocol library file
#ifdef _WIN32 //If compiled for Windows
	return LoadLibrary(libPath.c_str()); //Load using Windows API
#else //If not compiled for Windows
	lib handle = dlopen(libPath.c_str(), RTLD_LAZY); //Load using Unix API
	if (handle == NULL) { //If it didn't load
		error(dlerror()); //Print the error
		return nullptr; //Return null
	}
	return handle; //Load using C++ standard API
#endif
}

void registerVersion(Version* registee) { //Register a version in the database
	if (registered.count(registee->vers.major)) { //If the major version already exists
		Version* test = registered[registee->vers.major]; //Grab the already registered version
		if (test->vers.minor < registee->vers.minor) { //If the new version has a bigger minor version
			registered[registee->vers.major] = registee; //Replace the old version
		}
		else if (test->vers.patch < registee->vers.patch && test->vers.minor == registee->vers.minor){ //If the new version is the same minor version and has a bigger patch version
			registered[registee->vers.major] = registee; //Replace the old version
		}
		return; //We're done here
	}
	registered[registee->vers.major] = registee; //Add the version to the database
}

//PUBLIC
Status loadLibs() { //Load library files from apis subfolder
	path libDir = current_path(); //Grab the relative path
	libDir += "/apis";  //Navigate to the apis subfolder
	if (!exists(libDir)) { //If the subfolder is missing
		debug("Library search path: " + libDir.string()); //Debug print where exactly we are looking
		return Status(StatusCodes::NO_LIBRARY, "API Library Folder Not Found"); //Return that we've found nothing
	}
	for (directory_iterator iter(libDir); iter != end(iter); iter++) { //Continue until file list is equal to empty list
		directory_entry testFile = *iter; //Get file from list
		path testPath = testFile.path(); //Get path of file from list
		if (testPath.extension() == ".gelib") { //Test that file is a library file via file extension
			lib handle = loadLib(testPath); //Load library file
			if (handle == nullptr) { //If the file failed to load
				error("Failed to load " + testPath.filename().string()); //Print that we've failed
				continue; //Skip loading the file
			}
			Version* api = new Version(); //Create a new version object
			api->vers.major = *(UCHAR*)getValue(&handle, "major"); //Populate the version numbers
			api->vers.minor = *(UCHAR*)getValue(&handle, "minor");
			api->vers.patch = *(UCHAR*)getValue(&handle, "patch");
			api->procGate = (bool(*)(Gateway*))getValue(&handle, "processGateway"); //Populate the processing functions
			api->procPeer = (bool(*)(Peer*))getValue(&handle, "processGEDS");
			api->killGate = (void(*)(Gateway*))getValue(&handle, "killGateway"); //Populate the cleanup functions
			api->killPeer = (void(*)(Peer*))getValue(&handle, "killGEDS");
			if (api->procGate == nullptr || api->procPeer == nullptr || api->killGate == nullptr || api->killPeer == nullptr) { //If any function failed to load
				error("Failed to load " + testPath.filename().string()); //Print that we failed to load
				delete api; //Delete the new version to clear memory
				continue; //Skip the rest of loading
			}

			//void (*importFunc)(void*) = (void (*)(void*)) getValue(handle, "importFuncs"); //Experimental fix for Windows issues
			//importFunc(genPointers()); //Experimental fix
			registerVersion(api); //Register API
			log("Loaded " + testPath.stem().string() + " with version " + api->vers.stringify()); //Print that we've loaded
		}
	}
	if (registered.empty()) //If we didn't register anything
		return Status(StatusCodes::NO_LIBRARY, "No API Libraries Loaded"); //Return the empty error code
	int test = errno;
	return Status(StatusCodes::OK); //Return that we succeeded
}

Version* getVersion(UCHAR major) { //Get a version by the major number
	if (registered.count(major) != 0) { //If that version is registered
		return registered[major]; //Return it
	}
	return nullptr; //Else return predictable null
}

UCHAR highestVersion() { //Get the highest version registered
	map<UCHAR, Version*>::iterator last = registered.end(); //Go to the end of the list because it's sorted
	return (--last)->first; //Move back one (onto the list) and return that version
}
