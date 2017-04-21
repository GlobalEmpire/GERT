enum libErrors {
	NORMAL,
	UNKNOWN,
	EMPTY
}

struct versioning {
	UCHAR major, minor, patch;
}

class version {
	public:
		versioning vers;
		lib* handle;
		bool processGateway(gateway, string);
		bool processGEDS(geds, string);
		void killGateway(gateway);
		void killGEDS(geds);
	private:
		version(void);
}

int loadLibs();
version* getVersion(UCHAR);
