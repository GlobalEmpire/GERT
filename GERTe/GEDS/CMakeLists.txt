cmake_minimum_required(VERSION 3.10)

project(GEDS VERSION 2.0)

add_executable(GEDS "GEDS Server.cpp"
        Util/Error.cpp Util/logging.cpp Util/Trace.cpp
        Threading/Poll.cpp Threading/Processor.cpp
        Peer/Peer.cpp Peer/peerManager.cpp Peer/query.cpp
        Networking/Connection.cpp Networking/NetString.cpp Networking/netty.cpp
        Gateway/Address.cpp Gateway/gatewayManager.cpp Gateway/Key.cpp Gateway/RGateway.cpp Gateway/routeManager.cpp Gateway/Gateway.cpp
        Files/fileMngr.cpp
        )
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

    add_library(libcrypto SHARED IMPORTED)
    add_dependencies(libcrypto OpenSSLBuild)
    target_include_directories(libcrypto INTERFACE ../openssl/include/)
    target_link_libraries(libcrypto INTERFACE ../openssl/)

    target_link_libraries(GEDS PRIVATE libcrypto)
else()
    message(FATAL_ERROR "Cannot build OpenSSL, Perl was not found")
endif()


