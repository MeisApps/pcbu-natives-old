cmake_minimum_required(VERSION 3.14)

project(pcbu_auth)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL 3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libxcrypt IMPORTED_TARGET REQUIRED libxcrypt)
add_subdirectory(common-cpp/deps/spdlog)

add_compile_definitions(LINUX)
add_executable(pcbu_auth
        src/main.cpp

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
target_include_directories(pcbu_auth PRIVATE common-cpp/src common-cpp/deps/headers common-cpp/deps/spdlog/include)
target_link_libraries(pcbu_auth PRIVATE pthread bluetooth spdlog::spdlog PkgConfig::libxcrypt OpenSSL::Crypto OpenSSL::SSL)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
