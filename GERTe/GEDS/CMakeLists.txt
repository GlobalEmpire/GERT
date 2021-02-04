cmake_minimum_required(VERSION 3.10)

project(GEDS VERSION 2.0)

add_executable(GEDS "GEDS Server.cpp"
        Util/Error.cpp Util/logging.cpp Util/Trace.cpp
        Threading/Poll.cpp Threading/Processor.cpp
        Peer/Peer.cpp Peer/peerManager.cpp Peer/query.cpp
        Networking/Connection.cpp Networking/NetString.cpp Networking/netty.cpp
        Gateway/Address.cpp Gateway/Gateway.cpp Gateway/gatewayManager.cpp Gateway/Key.cpp Gateway/RGateway.cpp Gateway/routeManager.cpp Gateway/UGateway.cpp
        Files/fileMngr.cpp
        )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
