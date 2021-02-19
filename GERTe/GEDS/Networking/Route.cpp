#include "Route.h"

bool Route::operator==(const Route &other) const {
    return other.connection == connection;
}

void Route::registerRoute(const Address& addr, const Route& route, bool direct) {
    if (!routes.contains(addr))
        routes[addr] = DualStack();

    if (direct)
        routes[addr].directs.push_back(route);
    else
        routes[addr].indirects.push_back(route);
}

void Route::unregisterRoute(const Address& addr, const Connection* conn) {
    if (!routes.contains(addr))
        return;

    bool removed = false;
    for (auto route: routes[addr].directs)
        if (route.connection == conn) {
            routes[addr].directs.remove(route);
            removed = true;
            break;
        }

    if (!removed)
        for (auto route: routes[addr].indirects)
            if (route.connection == conn) {
                routes[addr].directs.remove(route);
                break;
            }

    if (routes[addr].directs.empty() && routes[addr].indirects.empty())
        routes.erase(addr);
}

Route* Route::getRoute(const Address& addr) {
    if (!routes.contains(addr))
        return nullptr;

    if (routes[addr].directs.empty())
        return &routes[addr].indirects.front();
    else
        return &routes[addr].directs.front();
}

bool Route::directRoute(const Address& addr) {
    if (!routes.contains(addr))
        return false;
    else
        return !routes[addr].directs.empty();
}
