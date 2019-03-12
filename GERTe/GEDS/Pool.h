typedef void(*POOLFUNC)(void*);

class Pool {
	unsigned int threads;
	void * arr;

public:
	Pool(POOLFUNC, void*);
	~Pool();

	void interrupt();
};
