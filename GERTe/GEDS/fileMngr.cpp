#define _CRT_SECURE_NO_WARNINGS //Windows grumble grumble

#include "peerManager.h" //Include peer manager so that we can write to the peer database
#include "keyMngr.h" //Include key manager so we can write to the key database

typedef map<GERTaddr, GERTkey>::iterator keyIter; //Define what a iterator for keys is

enum errors { //Define a list of error codes
	OK, //No error has occurred
	NOK //Error has occurred
};

extern char * LOCAL_IP; //Grab the local address

extern map<GERTaddr, GERTkey> resolutions; //Grab the key database

int loadPeers() { //Load peers from a file
	FILE* peerFile = fopen("peers.geds", "rb"); //Open the file in binary mode
	if (peerFile == nullptr) //If the file failed to open
		return NOK; //Return with an error
	while (true) {
		unsigned long ip; //Define a storage variable for an address
		fread(&ip, 4, 1, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2? Store an IP into previous variable
		if (feof(peerFile) != 0) //If the file has nothing left
			break; //We're done
		USHORT rawPorts[2]; //Make two storage variables for ports
		fread(&rawPorts, 2, 2, peerFile); //Fill those variables
		rawPorts[0] = ntohs(rawPorts[0]); //Fix those variables for platform compatibility
		rawPorts[1] = ntohs(rawPorts[1]);
		portComplex ports = {rawPorts[0], rawPorts[1]}; //Reformat those variables into a structure
		ipAddr ipClass = ip; //Reformat the address into a structure
		if (ipClass.stringify() != LOCAL_IP) //If the string version of the address isn't what the local address is set to
			log("Importing peer " + ipClass.stringify() + ":" + ports.stringify()); //Print out what we've imported
		addPeer(ipClass, ports); //Add peer to the database
	}
	fclose(peerFile); //Close the file
	return OK; //Return without errors
}

int loadResolutions() { //Load key resolutions from a file
	FILE* resolutionFile = fopen("resolutions.geds", "rb"); //Open the file in binary mode
	if (resolutionFile == nullptr) //If the file failed to open
		return NOK; //Return with an error
	while (true) {
		UCHAR bufE[3]; //Create a storage variable for the external portion of the address
		UCHAR bufI[3]; //Create a storage variable for the internal portion of the address
		fread(&bufE, 1, 3, resolutionFile); //Fill the external address
		fread(&bufI, 1, 3, resolutionFile); //Fill the internal address
		if (feof(resolutionFile) != 0) //If the file is at the end
			break; //We're done
		GERTaddr addr{bufE, bufI}; //Reformat the address portions into a single structure
		char buff[20]; //Create a storage variable for the key
		fread(&buff, 1, 20, resolutionFile); //Fill the key
		GERTkey key(buff); //Reformat the key
		log("Imported resolution for " + addr.stringify()); //Print what we've imported
		addResolution(addr, key); //Add the key to the database
	}
	fclose(resolutionFile); //Close the file
	return OK; //Return without an error
}

void savePeers() { //Save the database to a file
	FILE * peerFile = fopen("peers.geds", "wb"); //Open the file in binary mode
	for (knownIter iter; !iter.isEnd(); iter++) { //For each peer in the database
		knownPeer tosave = *iter; //Gets the next peer
		unsigned long addr = (unsigned long)tosave.addr.addr.s_addr; //Grabs the IP address and converts it to cross-platform mode
		fwrite(&addr, 4, 1, peerFile); //Writes it to file
		unsigned short gateport = tosave.ports.gate; //Converts gateway port to cross-platform mode
		fwrite(&gateport, 2, 1, peerFile); //Writes it to file
		unsigned short peerport = tosave.ports.peer; //Converts peer port to cross-platform mode
		fwrite(&peerport, 2, 1, peerFile); //Write it to file
	}
	fclose(peerFile); //Close the file
}

void saveResolutions() { //Save key resolutions to file
	FILE * resolutionFile = fopen("resolutions.geds", "wb"); //Open file in binary mode
	for (keyIter iter = resolutions.begin(); iter != resolutions.end(); iter++) { //For each key in the database
		GERTaddr addr = iter->first; //Get the associated address
		GERTkey key = iter->second; //Get the key
		fwrite(&addr.eAddr, 1, 3, resolutionFile); //Write the external address portion to file
		fwrite(&addr.iAddr, 1, 3, resolutionFile); //Write the internal address portion to file
		fwrite(&key.key, 1, 20, resolutionFile); //Write the key to file
	}
	fclose(resolutionFile); //Close the file
}
