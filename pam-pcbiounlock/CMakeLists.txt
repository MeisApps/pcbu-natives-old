cmake_minimum_required(VERSION 3.14)

project(pam_pcbiounlock)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenSSL 3 REQUIRED)

set(SPDLOG_BUILD_PIC ON CACHE BOOL "")
add_subdirectory(common-cpp/deps/spdlog)

if(APPLE)
    add_compile_definitions(APPLE)
    set(PLATFORM_LIBS)
elseif(UNIX)
    add_compile_definitions(LINUX)
    set(PLATFORM_LIBS bluetooth)
endif()

add_library(pam_pcbiounlock SHARED
        src/pam_auth.cpp
        src/pam_account.c
        src/pam_password.c
        src/pam_session.c

        common-cpp/src/api/api.h
        common-cpp/src/api/api.cpp
        common-cpp/src/api/bt_api.h
        common-cpp/src/api/bt_api.cpp

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
target_include_directories(pam_pcbiounlock PRIVATE common-cpp/src common-cpp/deps/headers common-cpp/deps/spdlog/include)
target_link_libraries(pam_pcbiounlock PRIVATE pam spdlog::spdlog OpenSSL::Crypto OpenSSL::SSL ${PLATFORM_LIBS})
