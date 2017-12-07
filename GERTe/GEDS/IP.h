#pragma once
#include <string>
#include <netinet/ip.h>
#include "Connection.h"
using namespace std;

class IP {
	public:
		in_addr addr;
		bool operator< (IP comp) const { return (addr.s_addr < comp.addr.s_addr); };
		bool operator== (IP comp) const { return (addr.s_addr == comp.addr.s_addr); };
		IP(unsigned long target) : addr(*(in_addr*)&target) {};
		IP(in_addr target) : addr(target) {};
		IP(string target) : addr(*(in_addr*)target.c_str()) {};
		string stringify() {
			unsigned char* rep = (unsigned char*)&addr.s_addr;
			return to_string(rep[0]) + "." + to_string(rep[1]) + "." + to_string(rep[2]) + "." + to_string(rep[3]);
		};
		IP static extract(Connection * conn) {
			char * raw = conn->read(4);
			string format = string{raw + 1, 4};
			delete raw;

			return IP{format};
		}
};
