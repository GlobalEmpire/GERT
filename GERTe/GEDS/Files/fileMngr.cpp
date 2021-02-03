#define _CRT_SECURE_NO_WARNINGS //Windows grumble grumble

#include "fileMngr.h"
#include "../Gateway/Key.h"
#include "../Util/logging.h"
#include "../Peer/peerManager.h"

typedef std::map<Address, Key>::iterator keyIter; //Define what a iterator for keys is

enum errors { //Define a list of error codes
	OK, //No error has occurred
	NOK //Error has occurred
};

extern char * LOCAL_IP; //Grab the local address

extern std::map<Address, Key> resolutions; //Grab the key database
extern std::map<IP, Ports> peerList;

int loadPeers() { //Load peers from a file
	FILE* peerFile = fopen("peers.geds", "rb"); //Open the file in binary mode
	if (peerFile == nullptr) { //If the file failed to open
		error("Failed to open peers file: " + std::to_string(errno));
		return -1; //Return with an error
	}

	while (true) {
		uint32_t ip; //Define a storage variable for an address
		fread(&ip, 4, 1, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2? Store an IP into previous variable
		if (feof(peerFile) != 0) //If the file has nothing left
			break; //We're done
		unsigned short rawPorts[2]; //Make two storage variables for ports
		fread(rawPorts, 2, 2, peerFile); //Fill those variables
		rawPorts[0] = rawPorts[0]; //Fix those variables for platform compatibility
		rawPorts[1] = rawPorts[1];
		Ports ports = {rawPorts[0], rawPorts[1]}; //Reformat those variables into a structure
		IP ipClass = ip; //Reformat the address into a structure
		if (ipClass.stringify() != LOCAL_IP) //If the string version of the address isn't what the local address is set to
			log("Importing peer " + ipClass.stringify() + ":" + ports.stringify()); //Print out what we've imported
		Peer::allow(ipClass, ports); //Add peer to the database
	}

	fclose(peerFile); //Close the file
	return 0; //Return without errors
}

int loadResolutions() { //Load key resolutions from a file
	FILE* resolutionFile = fopen("resolutions.geds", "rb"); //Open the file in binary mode
	if (resolutionFile == nullptr) { //If the file failed to open
		error("Failed to open key file: " + std::to_string(errno));
		return -1;
	}

	while (true) {
		unsigned char bufE[3]; //Create a storage variable for the external portion of the address
		fread(bufE, 1, 3, resolutionFile); //Fill the external address
		if (feof(resolutionFile) != 0) //If the file is at the end
			break; //We're done
		Address addr{bufE}; //Reformat the address portions into a single structure
		char buff[20]; //Create a storage variable for the key
		fread(buff, 1, 20, resolutionFile); //Fill the key
		Key key(buff); //Reformat the key
		log("Imported resolution for " + addr.stringify()); //Print what we've imported
		resolutions[addr] = key;
	}

	fclose(resolutionFile); //Close the file
	return 0; //Return without an error
}

void savePeers() { //Save the database to a file
	FILE * peerFile = fopen("peers.geds", "wb"); //Open the file in binary mode
	for (std::map<IP, Ports>::iterator iter = peerList.begin(); iter != peerList.end(); iter++) { //For each peer in the database
		IP addr = iter->first; //Gets the next peer
		Ports ports = iter->second;
		fwrite(&addr.addr.s_addr, 4, 1, peerFile); //Writes it to file
		unsigned short gateport = ports.gate; //Converts gateway port to cross-platform mode
		fwrite(&gateport, 2, 1, peerFile); //Writes it to file
		unsigned short peerport = ports.peer; //Converts peer port to cross-platform mode
		fwrite(&peerport, 2, 1, peerFile); //Write it to file
	}
	fclose(peerFile); //Close the file
}

void saveResolutions() { //Save key resolutions to file
	FILE * resolutionFile = fopen("resolutions.geds", "wb"); //Open file in binary mode
	for (keyIter iter = resolutions.begin(); iter != resolutions.end(); iter++) { //For each key in the database
		Address addr = iter->first; //Get the associated address
		Key key = iter->second; //Get the key
		fwrite(addr.getAddr(), 1, 3, resolutionFile); //Write the external address portion to file
		fwrite(key.key.c_str(), 1, 20, resolutionFile); //Write the key to file
	}
	fclose(resolutionFile); //Close the file
}
