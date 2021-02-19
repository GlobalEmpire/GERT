#pragma once
#include "../Gateway/Address.h"
#include "Connection.h"
#include <stdexcept>
#include <string>
#include <list>
#include <map>

class Route {
    struct DualStack {
        std::list<Route> directs;
        std::list<Route> indirects;
    };

    inline static std::map<Address, DualStack> routes;

public:
    unsigned char major = 0;
    unsigned char minor = 0;
    Connection* connection = nullptr;

    bool operator==(const Route& other) const;

    static void registerRoute(const Address&, const Route&, bool);
    static void unregisterRoute(const Address&, const Connection*);
    static Route* getRoute(const Address&);
    static bool directRoute(const Address&);
};
