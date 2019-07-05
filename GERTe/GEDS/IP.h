#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/ip.h>
#endif

#include "Connection.h"
#include <cstring>

class IP {
	public:
		in_addr addr;
		bool operator< (IP comp) const { return (addr.s_addr < comp.addr.s_addr); };
		bool operator== (IP comp) const { return (addr.s_addr == comp.addr.s_addr); };

		IP(unsigned long target = 0) : addr(*(in_addr*)&target) {};
		IP(in_addr target) : addr(target) {};
		IP(std::string target) : addr(*(in_addr*)target.c_str()) {};
		IP(Connection * conn, int offset) {
			memcpy(&addr, conn->buf + offset, 4);
		}

		std::string stringify() {
			unsigned char* rep = (unsigned char*)&addr;
			return std::to_string(rep[0]) + "." + std::to_string(rep[1]) + "." + std::to_string(rep[2]) + "." + std::to_string(rep[3]);
		};
};
