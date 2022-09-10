cmake_minimum_required(VERSION 3.14)

project(pcbu_auth)
set(CMAKE_CXX_STANDARD 17)

include_directories(../common-cpp/)

find_package(OpenSSL 1.1 REQUIRED)

add_executable(pcbu_auth main.cpp ../common-cpp/servers/tcp/UnlockServer.cpp ../common-cpp/FCMUtils.cpp ../common-cpp/handler/KeyScanner.cpp ../common-cpp/handler/KeyScanner.h ../common-cpp/AppStorage.h ../common-cpp/servers/BaseUnlockServer.h ../common-cpp/clients/BTUnlockClient.cpp ../common-cpp/clients/BTUnlockClient.h ../common-cpp/BTUtils.h ../common-cpp/api/bt_api.cpp ../common-cpp/api/bt_api.h ../common-cpp/api/api.h ../common-cpp/api/api.cpp ../common-cpp/clients/TCPUnlockClient.cpp ../common-cpp/clients/TCPUnlockClient.h ../common-cpp/handler/UnlockHandler.cpp ../common-cpp/handler/UnlockHandler.h ../common-cpp/handler/PairedDevice.h ../common-cpp/I18n.h ../common-cpp/I18n.cpp)
target_link_libraries(pcbu_auth PRIVATE pthread crypt bluetooth OpenSSL::Crypto OpenSSL::SSL)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
