#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
typedef HMODULE lib;
#include <Windows.h>
#else
#include <dlfcn.h>
typedef void* lib;
#endif
#include <filesystem>
#include <map>
typedef unsigned char UCHAR;
using namespace experimental::filesystem::v1;

map<UCHAR, version> registered;

//PUBLIC
enum libErrors {
	NORMAL,
	UNKNOWN,
	EMPTY
}

//CAN BE PUBLIC
struct versioning {
	UCHAR major, minor, patch;
}

//PUBLIC
class version {
	public:
		versioning vers;
		lib* handle;
		bool processGateway(gateway, string);
		bool processGEDS(geds, string);
		void killGateway(gateway);
		void killGEDS(geds);
}

//PUBLIC
version::version(lib loaded) : handle(&loaded) {
	vers.major = (UCHAR)getValue(handle, "major");
	vers.minor = (UCHAR)getValue(handle, "minor");
	vers.patch = (UCHAR)getValue(handle, "patch");
	processGateway = getValue(handle, "processGateway");
	processGEDS = getValue(handle, "processGEDS");
	killGateway = getValue(handle, "killGateway");
	killGEDS = getValue(handle, "killGEDS");
}

void* getValue(lib* handle, string value) { //Retrieve gelib value
#ifdef _WIN32 //If compiled for Windows
	return GetProcAddress(*handle, value.c_str()); //Retrieve using Windows API
#else //If not compiled for Windows
	return dlsym(handle, value.c_str()); //Retrieve using C++ standard API
#endif
}

lib loadLib(path libPath) { //Load gelib file
#ifdef _WIN32 //If compiled for Windows
	return LoadLibrary(libPath.c_str()); //Load using Windows API
#else //If not compiled for Windows
	return dlload(libPath.c_str(), RTLD_LAZY); //Load using C++ standard API
#endif
}

void registerVersion(version registee) {
	if (registered.count(registee.major)) {
		version test = knownVersions[registee.major];
		if (test.minor < registee.minor) {
			registered[registee.major] = registee;
		}
		else if (test.patch < registee.patch && test.minor == registee.minor){
			registered[registee.major] = registee;
		}
		return;
	}
	registered[registee.major] = registee;
}

//PUBLIC
int loadLibs() { //Load gelib files from api subfolder
	path libDir = "./apis/"; //Define relative path to subfolder
	directory_iterator iter(libDir), end; //Define file list and empty list
	while (iter != end) { //Continue until file list is equal to empty list
		directory_entry testFile = *iter; //Get file from list
		path testPath = testFile.path(); //Get path of file from list
		if (testPath.extension == "gelib") { //Test that file is a gelib file via file extension
			lib handle = loadLib(testPath); //Load gelib file
			version api(handle);
			registerVersion(api); //Register API and map API
		}
		iter++; //Move to next file
	}
	if (registered.empty())
		return EMPTY;
	return NORMAL;
}

//PUBLIC
version* getVersion(UCHAR major) {
	if (registered.count(major)) {
		return &registered[major]
	}
	return nullptr;
}
