#pragma once
#include <string>

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

class DataPacket;

class Connection {
protected:
	Connection(SOCKET, const std::string&);
	explicit Connection(SOCKET);

	void close();

public:
	virtual ~Connection();

	SOCKET sock;
	unsigned char vers[2] = {0, 0};
	bool valid = false;

	virtual void process() = 0;
	virtual void send(const DataPacket&) = 0;

	[[nodiscard]] std::string read(int=1) const;
	void write(const std::string&) const;
};
