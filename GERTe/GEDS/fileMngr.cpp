#include "logging.h"
#include "netty.h"
#include "keyMngr.h"
#include <map>

typedef map<ipAddr, knownPeer>::iterator peerIter;
typedef map<GERTaddr, GERTkey>::iterator keyIter;

enum errors {
	NORMAL,
	ERROR
};

extern map<ipAddr, knownPeer> peerList;
extern map<GERTaddr, GERTkey> resolutions;

int loadPeers() {
	FILE* peerFile = fopen("peers.geds", "rb");
	if (peerFile == nullptr)
		return ERROR;
	while (true) {
		UCHAR ip[4];
		fread(&ip, 1, 4, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2?
		if (feof(peerFile) != 0)
			break;
		USHORT rawPorts[2];
		fread(&rawPorts, 2, 2, peerFile);
		rawPorts[0] = ntohs(rawPorts[0]);
		rawPorts[1] = ntohs(rawPorts[1]);
		portComplex ports = {rawPorts[0], rawPorts[1]};
		ipAddr ipClass = ip;
		log("Importing peer " + ipClass.stringify() + ":" + ports.stringify());
		addPeer(ipClass, ports);
	}
	fclose(peerFile);
	return NORMAL;
}

int loadResolutions() {
	FILE* resolutionFile = fopen("resolutions.geds", "rb");
	if (resolutionFile == nullptr)
		return ERROR;
	while (true) {
		USHORT buf[2];
		fread(&buf, 2, 2, resolutionFile);
		if (feof(resolutionFile) != 0)
			break;
		GERTaddr addr = {ntohs(buf[0]), ntohs(buf[1])};
		char buff[20];
		fread(&buff, 1, 20, resolutionFile);
		GERTkey key(buff);
		log("Imported resolution for " + to_string(addr.high) + "-" + to_string(addr.low));
		addResolution(addr, key);
	}
	fclose(resolutionFile);
	return NORMAL;
}

void savePeers() {
	FILE * peerFile = fopen("peers.geds", "wb");
	for (peerIter iter = peerList.begin(); iter != peerList.end(); iter++) {
		knownPeer tosave = iter->second;
		unsigned long addr = htonl((unsigned long)tosave.addr.addr.s_addr);
		fwrite(&addr, 4, 1, peerFile);
		unsigned short gateport = htons(tosave.ports.gate);
		fwrite(&gateport, 2, 1, peerFile);
		unsigned short peerport = htons(tosave.ports.peer);
		fwrite(&peerport, 2, 1, peerFile);
	}
	fclose(peerFile);
}

void saveResolutions() {
	FILE * resolutionFile = fopen("resolutions.geds", "wb");
	for (keyIter iter = resolutions.begin(); iter != resolutions.end(); iter++) {
		GERTaddr addr = iter->first;
		GERTkey key = iter->second;
		USHORT high = htons(addr.high);
		USHORT low = htons(addr.low);
		fwrite(&high, 2, 1, resolutionFile);
		fwrite(&low, 2, 1, resolutionFile);
		fwrite(&key.key, 1, 20, resolutionFile);
	}
	fclose(resolutionFile);
}
