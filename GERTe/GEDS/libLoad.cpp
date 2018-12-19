#include <map> //Load map type for databases
#include "netty.h" //Load netty header for ... Reason? Notify me if you determine what is required
#include "libLoad.h"
#include "logging.h"
typedef unsigned char UCHAR;
using namespace std; //Default to using STD namespace

extern Version ThisVersion;

//PUBLIC
Version* getVersion(UCHAR major) { //Get a version by the major number
	if (major == ThisVersion.vers.major) { //If that version is registered
		return &ThisVersion; //Return it
	}
	return nullptr; //Else return predictable null
}

UCHAR highestVersion() { //Get the highest version registered
	return ThisVersion.vers.major;
}
