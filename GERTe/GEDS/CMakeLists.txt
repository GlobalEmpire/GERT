cmake_minimum_required(VERSION 3.10)

project(GEDS VERSION 2.0)

set(CMAKE_CXX_STANDARD 20)

add_executable(GEDS "GEDS Server.cpp"
        Util/Error.cpp Util/logging.cpp Util/Trace.cpp
        Threading/Poll.cpp Threading/Processor.cpp
        Peer/query.cpp
        Networking/Connection.cpp Networking/NetString.cpp Networking/netty.cpp
        Gateway/Address.cpp Gateway/Key.cpp
        Files/fileMngr.cpp
        Gateway/DataConnection.cpp Gateway/DataConnection.h Util/Crypto.cpp Util/Crypto.h Gateway/DataPacket.cpp Gateway/DataPacket.h Peer/CommandConnection.cpp Peer/CommandConnection.h Networking/Route.cpp Networking/Route.h Peer/CommandPacket.cpp Peer/CommandPacket.h Networking/ReentrantPacket.cpp Networking/ReentrantPacket.h Peer/RelayPacket.cpp Peer/RelayPacket.h Peer/QueryPacket.cpp Peer/QueryPacket.h Peer/QueryPPacket.cpp Peer/QueryPPacket.h)

target_include_directories(GEDS PRIVATE ../openssl/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FindPerl)

if (PERL_FOUND)
    include(ExternalProject)

    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P MATCHES 8)
            set(OPENSSL_CONFIGURE_TARGET VC-WIN64A-masm)
        else()
            set(OPENSSL_CONFIGURE_TARGET VC-WIN32)
        endif()

        set(OPENSSL_CONFIGURE ${PERL_EXECUTABLE} Configure ${OPENSSL_CONFIGURE_TARGET})
    else()
        set(OPENSSL_CONFIGURE ./config)
    endif()

    ExternalProject_Add(
            OpenSSLBuild
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openssl/
            BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openssl/
            CONFIGURE_COMMAND ${OPENSSL_CONFIGURE}
            BUILD_COMMAND ${MAKE_EXE}
            INSTALL_COMMAND ""
    )

    add_dependencies(GEDS OpenSSLBuild)
    target_link_directories(GEDS PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../openssl/")
    target_link_libraries(GEDS PRIVATE "libcrypto")
    target_include_directories(GEDS PRIVATE ../openssl/include/)
else()
    message(FATAL_ERROR "Cannot build OpenSSL, Perl was not found")
endif()


