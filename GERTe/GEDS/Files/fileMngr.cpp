#define _CRT_SECURE_NO_WARNINGS //Windows grumble grumble

#include "fileMngr.h"
#include "../Gateway/Key.h"
#include "../Util/logging.h"
#include <map>

typedef std::map<Address, Key>::iterator keyIter; //Define what a iterator for keys is

enum errors { //Define a list of error codes
	OK, //No error has occurred
	NOK //Error has occurred
};

extern char * LOCAL_IP; //Grab the local address

bool loadResolutions() { //Load key resolutions from a file
	FILE* resolutionFile = fopen("resolutions.geds", "rb"); //Open the file in binary mode
	if (resolutionFile == nullptr) { //If the file failed to open
		error("Failed to open key file: " + std::to_string(errno));
		return false;
	}

	while (true) {
		unsigned char bufE[3]; //Create a storage variable for the external portion of the address
		fread(bufE, 1, 3, resolutionFile); //Fill the external address
		if (feof(resolutionFile) != 0) //If the file is at the end
			break; //We're done

		Address addr{std::string{ (char*)bufE, 3 }}; //Reformat the address portions into a single structure

		char buff[20]; //Create a storage variable for the key
		fread(buff, 1, 20, resolutionFile); //Fill the key
		Key key(buff); //Reformat the key

		log("Imported resolution for " + addr.stringify()); //Print what we've imported
		Key::resolutions.at(addr) = key;
	}

	fclose(resolutionFile); //Close the file
	return true; //Return without an error
}
