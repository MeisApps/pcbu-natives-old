cmake_minimum_required(VERSION 3.14)

project(win-pcbiounlock)
set(CMAKE_CXX_STANDARD 17)

message("-- Building for arch '${CMAKE_VS_PLATFORM_NAME}'")
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64-ARM")
else()
    #set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64-ARM")
endif()

find_package(OpenSSL 3 REQUIRED)
add_subdirectory(common-cpp/deps/spdlog)

add_compile_definitions(_WIN32 WIN32 SAMPLEV2CREDENTIALPROVIDER_EXPORTS _WINDOWS _USRDLL UNICODE _UNICODE)
add_library(win-pcbiounlock SHARED
        src/win-pcbiounlock.def
        src/resource.rc
        src/common.h
        src/WinUtils.h
        src/guid.h
        src/guid.cpp
        src/Dll.h
        src/Dll.cpp
        src/helpers.h
        src/helpers.cpp
        src/CSampleProvider.h
        src/CSampleProvider.cpp
        src/CUnlockCredential.h
        src/CUnlockCredential.cpp
        src/CUnlockListener.h
        src/CUnlockListener.cpp

        common-cpp/src/api/api.h
        common-cpp/src/api/api.cpp
        common-cpp/src/api/bt_api.h
        common-cpp/src/api/bt_api.cpp

        common-cpp/src/connection/BaseUnlockServer.h
        common-cpp/src/connection/BaseUnlockServer.cpp

        common-cpp/src/connection/clients/TCPUnlockClient.h
        common-cpp/src/connection/clients/TCPUnlockClient.cpp
        common-cpp/src/connection/clients/BTUnlockClient.h
        common-cpp/src/connection/clients/BTUnlockClient.cpp
        common-cpp/src/connection/servers/TCPUnlockServer.h
        common-cpp/src/connection/servers/TCPUnlockServer.cpp

        common-cpp/src/handler/KeyScanner.h
        common-cpp/src/handler/KeyScanner.cpp
        common-cpp/src/handler/UnlockHandler.h
        common-cpp/src/handler/UnlockHandler.cpp

        common-cpp/src/storage/AppStorage.h
        common-cpp/src/storage/AppStorage.cpp
        common-cpp/src/storage/PairedDevice.h
        common-cpp/src/storage/PairedDevice.cpp

        common-cpp/src/utils/Utils.h
        common-cpp/src/utils/Utils.cpp
        common-cpp/src/utils/CryptUtils.h
        common-cpp/src/utils/CryptUtils.cpp
        common-cpp/src/utils/BTUtils.h
        common-cpp/src/utils/BTUtils.cpp

        common-cpp/src/I18n.h
        common-cpp/src/I18n.cpp
        common-cpp/src/Logger.h
        common-cpp/src/Logger.cpp
)

target_include_directories(win-pcbiounlock PUBLIC common-cpp/src common-cpp/deps/headers common-cpp/deps/spdlog/include)
target_link_libraries(win-pcbiounlock PUBLIC Credui Shlwapi Secur32 Ws2_32 Bthprops spdlog::spdlog OpenSSL::Crypto OpenSSL::SSL)
