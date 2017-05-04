#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
typedef HMODULE lib;
#include <Windows.h>
#include <filesystem>
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

map<UCHAR, version> registered;

enum libErrors {
	NO_ERR,
	UNKNOWN,
	EMPTY
};

void* getValue(void* handle, string value) { //Retrieve gelib value
#ifdef _WIN32 //If compiled for Windows
	return GetProcAddress(*(lib*)handle, value.c_str()); //Retrieve using Windows API
#else //If not compiled for Windows
	return dlsym(*(lib*)handle, value.c_str()); //Retrieve using C++ standard API
#endif
}

lib loadLib(path libPath) { //Load gelib file
#ifdef _WIN32 //If compiled for Windows
	return LoadLibrary(libPath.c_str()); //Load using Windows API
#else //If not compiled for Windows
	return dlopen(libPath.c_str(), RTLD_LAZY); //Load using C++ standard API
#endif
}

void registerVersion(version registee) {
	if (registered.count(registee.vers.major)) {
		version test = registered[registee.vers.major];
		if (test.vers.minor < registee.vers.minor) {
			registered[registee.vers.major] = registee;
		}
		else if (test.vers.patch < registee.vers.patch && test.vers.minor == registee.vers.minor){
			registered[registee.vers.major] = registee;
		}
		return;
	}
	registered[registee.vers.major] = registee;
}

/*void* genPointers() {
	void* pointers = new [13];
	(bool ()(GERTaddr, string))pointers[0] = sendTo;
	(void (*)(gateway*, string))pointers[1] = sendTo;
	(void (*)(peer*, string))pointers[2] = sendTo;
	pointers[3] = assign;
	pointers[4] = isRemote;
	(void (*)(gateway*))pointers[5] = closeTarget;
	(void (*)(peer*))pointers[6] = closeTarget;
	pointers[7] = setRoute;
	pointers[8] = removeRoute;
	pointers[9] = addResolution;
	pointers[10] = removeResolution;
	pointers[11] = addPeer;
	pointers[12] = removePeer;
	return pointers;
}*/

//PUBLIC
int loadLibs() { //Load gelib files from api subfolder
	path libDir = current_path(); //Define relative path to subfolder
	libDir += "/apis";
	if (not exists(libDir)) {
		error("Can't find apis directory. DUMP: " + libDir.string());
		return EMPTY;
	}
	directory_iterator iter(libDir), end; //Define file list and empty list
	while (iter != end) { //Continue until file list is equal to empty list
		directory_entry testFile = *iter; //Get file from list
		path testPath = testFile.path(); //Get path of file from list
		if (testPath.extension() == ".gelib") { //Test that file is a gelib file via file extension
			lib handle = loadLib(testPath); //Load gelib file
			version api;
			api.vers.major = *(UCHAR*)getValue(&handle, "major");
			api.vers.minor = *(UCHAR*)getValue(&handle, "minor");
			api.vers.patch = *(UCHAR*)getValue(&handle, "patch");
			api.procGate = (bool(*)(gateway*, string))getValue(&handle, "processGateway");
			api.procPeer = (bool(*)(peer*, string))getValue(&handle, "processPeer");
			api.killGate = (void(*)(gateway*))getValue(&handle, "killGateway");
			api.killPeer = (void(*)(peer*))getValue(&handle, "killGEDS");
			//void (*importFunc)(void*) = (void (*)(void*)) getValue(handle, "importFuncs");
			//importFunc(genPointers());
			registerVersion(api); //Register API and map API
			log("Loaded " + testPath.string() + " with version " + api.vers.stringify());
		}
		iter++; //Move to next file
	}
	if (registered.empty())
		return EMPTY;
	return NO_ERR;
}

version* getVersion(UCHAR major) {
	if (registered.count(major) != 0) {
		return &(registered[major]);
	}
	return nullptr;
}

UCHAR highestVersion() {
	map<UCHAR, version>::iterator last = registered.end();
	return (--last)->first;
}
