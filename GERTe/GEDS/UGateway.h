#pragma once
#include "Connection.h"
#include "Key.h"

class Gateway;

#undef REGISTERED

namespace Gate {
	enum class Commands : char {
		STATE,
		REGISTER,
		DATA,
		CLOSE
	};

	enum class States : char {
		FAILURE,
		CONNECTED,
		REGISTERED,
		CLOSED,
		SENT
	};

	enum class Errors : char {
		VERSION,
		BAD_KEY,
		REGISTERED,
		NOT_REGISTERED,
		NO_ROUTE,
		ADDRESS_TAKEN
	};
}

class UGateway : public Connection { //The OG Gateway
	void changeState(const Gate::States, const char = 0, const char* = nullptr);

protected:
	UGateway(UGateway&&) noexcept;

public:
	UGateway(SOCKET);
	virtual ~UGateway();

	void transmit(std::string);
	bool assign(Address, Key);
	void close();
	void process(Gateway *);
	void process() { process(nullptr); };
};
