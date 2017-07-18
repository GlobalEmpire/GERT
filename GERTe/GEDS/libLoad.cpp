#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
#include <Windows.h>
#include <filesystem>
typedef HMODULE lib;
#else
#include <dlfcn.h>
#include <experimental/filesystem>
typedef void* lib;
#endif
#include <map>
#include <string>
#include "netty.h"
#include "logging.h"
typedef unsigned char UCHAR;
using namespace std;
using namespace experimental::filesystem::v1;

map<UCHAR, version*> registered;

enum libErrors {
	NO_ERR,
	UNKNOWN,
	EMPTY
};

void* getValue(void* handle, string value) { //Retrieve gelib value
#ifdef _WIN32 //If compiled for Windows
	return GetProcAddress(*(lib*)handle, value.c_str()); //Retrieve using Windows API
#else //If not compiled for Windows
	void * test = dlsym(*(lib*)handle, value.c_str());
	char * err = dlerror();
	if (err != nullptr) {
		error(err);
		return nullptr;
	}
	return test; //Retrieve using C++ standard API
#endif
}

lib loadLib(path libPath) { //Load gelib file
#ifdef _WIN32 //If compiled for Windows
	return LoadLibrary(libPath.c_str()); //Load using Windows API
#else //If not compiled for Windows
	lib handle = dlopen(libPath.c_str(), RTLD_LAZY);
	if (handle == NULL) {
		error(dlerror());
		return nullptr;
	}
	return handle; //Load using C++ standard API
#endif
}

void registerVersion(version* registee) {
	if (registered.count(registee->vers.major)) {
		version* test = registered[registee->vers.major];
		if (test->vers.minor < registee->vers.minor) {
			registered[registee->vers.major] = registee;
		}
		else if (test->vers.patch < registee->vers.patch && test->vers.minor == registee->vers.minor){
			registered[registee->vers.major] = registee;
		}
		return;
	}
	registered[registee->vers.major] = registee;
}

//PUBLIC
int loadLibs() { //Load gelib files from api subfolder
	path libDir = current_path(); //Define relative path to subfolder
	libDir += "/apis";
	if (!exists(libDir)) {
		error("Can't find apis directory.");
		debug("Library search path: " + libDir.string());
		return EMPTY;
	}
	for (directory_iterator iter(libDir); iter != end(iter); iter++) { //Continue until file list is equal to empty list
		directory_entry testFile = *iter; //Get file from list
		path testPath = testFile.path(); //Get path of file from list
		if (testPath.extension() == ".gelib") { //Test that file is a gelib file via file extension
			lib handle = loadLib(testPath); //Load gelib file
			if (handle == nullptr) {
				error("Failed to load " + testPath.filename().string());
				continue;
			}
			version* api = new version();
			api->vers.major = *(UCHAR*)getValue(&handle, "major");
			api->vers.minor = *(UCHAR*)getValue(&handle, "minor");
			api->vers.patch = *(UCHAR*)getValue(&handle, "patch");
			api->procGate = (bool(*)(gateway*, string))getValue(&handle, "processGateway");
			api->procPeer = (bool(*)(peer*, string))getValue(&handle, "processGEDS");
			api->killGate = (void(*)(gateway*))getValue(&handle, "killGateway");
			api->killPeer = (void(*)(peer*))getValue(&handle, "killGEDS");
			if (api->procGate == nullptr || api->procPeer == nullptr || api->killGate == nullptr || api->killPeer == nullptr) {
				error("Failed to load " + testPath.filename().string());
				delete api;
				continue;
			}

			//void (*importFunc)(void*) = (void (*)(void*)) getValue(handle, "importFuncs");
			//importFunc(genPointers());
			registerVersion(api); //Register API and map API
			log("Loaded " + testPath.stem().string() + " with version " + api->vers.stringify());
		}
	}
	if (registered.empty())
		return EMPTY;
	return NO_ERR;
}

version* getVersion(UCHAR major) {
	if (registered.count(major) != 0) {
		return registered[major];
	}
	return nullptr;
}

UCHAR highestVersion() {
	map<UCHAR, version*>::iterator last = registered.end();
	return (--last)->first;
}
