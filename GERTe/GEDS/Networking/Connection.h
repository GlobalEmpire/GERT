#pragma once
#include <string>

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

class Connection {
protected:
	Connection(SOCKET, const std::string&);
	explicit Connection(SOCKET);

	void error(char * err) const;

public:
	virtual ~Connection();

	SOCKET sock;
	unsigned char state = 0;
	unsigned char vers[2] = {0, 0};

	virtual void process() = 0;
	virtual void close() = 0;

	char* read(int=1) const;
	void write(const std::string&) const;
};
