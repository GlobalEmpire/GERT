cmake_minimum_required(VERSION 3.19)

project(GERTe VERSION 2.0 DESCRIPTION "Decentralized, secure, reliable network protocol server."
        HOMEPAGE_URL "https://github.com/GlobalEmpire/GERT")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC AND CMAKE_VERSION VERSION_LESS 3.20) # Cleanup some stupid CMake defaults
    string(REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/Ob2" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

include(BuildOptions.cmake)

add_executable(GEDS "GEDS Server.cpp"
        Util/Error.cpp Util/logging.cpp Util/Trace.cpp Util/Crypto.cpp Util/Crypto.h

        Threading/Poll.cpp Threading/Processor.cpp

        Peer/query.cpp Peer/CommandConnection.cpp Peer/CommandConnection.h Peer/CommandPacket.cpp Peer/CommandPacket.h
        Peer/RelayPacket.cpp Peer/RelayPacket.h Peer/QueryPacket.cpp Peer/QueryPacket.h Peer/QueryPPacket.cpp
        Peer/QueryPPacket.h

        Networking/Connection.cpp Networking/NetString.cpp Networking/netty.cpp Networking/Route.cpp Networking/Route.h
        Networking/ReentrantPacket.cpp Networking/ReentrantPacket.h

        Gateway/Address.cpp Gateway/Key.cpp Gateway/DataConnection.cpp Gateway/DataConnection.h Gateway/DataPacket.cpp
        Gateway/DataPacket.h

        Files/fileMngr.cpp)

if (MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(GEDS PRIVATE /Zi /Zf /MDd)
    else()
        target_compile_options(GEDS PRIVATE /Zi /O2 /MD)
    endif()

    target_compile_options(GEDS PRIVATE /GR- /Gw /Gy /GL /MP /GF /fp:fast)
    target_link_options(GEDS PRIVATE /LTCG)
endif()

if (NOT WIN32)
    target_link_libraries(GEDS PRIVATE pthread)
endif()

if (BUILD_OPENSSL)
    include(FindPerl)
    if (PERL_FOUND)
        include(ExternalProject)

        set(OPENSSL_CONFIG_OPTIONS no-shared no-deprecated)

        set(OPENSSL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../openssl)
        if(WIN32)
            message(WARNING
                    "If the build fails when trying to open some file, or PDB API call error 3, just try again.\n"
                    "If a second build fails with the same problem, contact Microsoft, it is not my fault.")

            if(CMAKE_SIZEOF_VOID_P MATCHES 8)
                set(OPENSSL_CONFIGURE_TARGET "VC-WIN64A-masm")
            else()
                set(OPENSSL_CONFIGURE_TARGET "VC-WIN32")
            endif()

            set(OPENSSL_CONFIGURE ${PERL_EXECUTABLE} ${OPENSSL_DIR}/Configure ${OPENSSL_CONFIG_OPTIONS} ${OPENSSL_CONFIGURE_TARGET})
            set(CRYPTO_TARGET libcrypto.lib)
            target_link_libraries(GEDS PRIVATE libcrypto crypt32)
        else()
            set(OPENSSL_CONFIGURE ${OPENSSL_DIR}/config ${OPENSSL_CONFIG_OPTIONS})
            set(CRYPTO_TARGET libcrypto.pc)
            target_link_libraries(GEDS PRIVATE crypto dl)
        endif()

        ExternalProject_Add(OpenSSLBuild
                SOURCE_DIR ${OPENSSL_DIR}

                # Download
                GIT_SUBMODULES ${OPENSSL_DIR}
                #PATCH_COMMAND $(MAKE) clean

                # Configure
                CONFIGURE_COMMAND ${OPENSSL_CONFIGURE}

                # Compile
                BUILD_COMMAND $(MAKE) build_generated
                COMMAND $(MAKE) depend
                COMMAND $(MAKE) ${CRYPTO_TARGET}

                # Install
                INSTALL_COMMAND "")

        add_dependencies(GEDS OpenSSLBuild)

        ExternalProject_Get_property(OpenSSLBuild BINARY_DIR)
        target_include_directories(GEDS PRIVATE ${BINARY_DIR}/include/)
        target_link_directories(GEDS PRIVATE ${BINARY_DIR})
        unset(BINARY_DIR)

        target_include_directories(GEDS PRIVATE ${OPENSSL_DIR}/include/)
    else()
        message(FATAL_ERROR "Cannot build OpenSSL, Perl was not found")
    endif()
else()
    if (EXISTS ${OPENSSL_DIR})
        if (EXISTS ${OPENSSL_DIR}/include)
            target_include_directories(GEDS PRIVATE ${OPENSSL_DIR}/include/)
        endif()

        if (WIN32)
            if (EXISTS ${OPENSSL_DIR}/libcrypto.dll)
                target_link_directories(GEDS PRIVATE ${OPENSSL_DIR}/openssl/)
                target_link_libraries(GEDS PRIVATE libcrypto crypt32)
            endif()
        else()
            if (EXISTS ${OPENSSL_DIR}/libcrypto.a)
                target_link_directories(GEDS PRIVATE ${OPENSSL_DIR}/openssl/)
                target_link_libraries(GEDS PRIVATE crypto dl)
            endif()
        endif()
    else()
        include(FindOpenSSL)
        if (OPENSSL_FOUND)
            target_include_directories(GEDS PRIVATE ${OPENSSL_INCLUDE_DIR})
            target_link_libraries(GEDS PRIVATE ${OPENSSL_CRYPTO_LIBRARIES})
        else()
            message(FATAL_ERROR "OpenSSL was not built, but no headers could be found.")
        endif()
    endif()
endif()
