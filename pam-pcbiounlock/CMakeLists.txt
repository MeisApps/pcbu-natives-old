cmake_minimum_required(VERSION 3.14)

project(pam_pcbiounlock)
set(CMAKE_CXX_STANDARD 17)

add_library(pam_pcbiounlock SHARED src/pam_auth.cpp src/pam_account.c src/pam_password.c src/pam_session.c common-cpp/BTUtils.h common-cpp/api/bt_api.cpp common-cpp/api/bt_api.h common-cpp/api/api.h common-cpp/api/api.cpp common-cpp/I18n.h common-cpp/I18n.cpp)
target_link_libraries(pam_pcbiounlock PRIVATE pam bluetooth)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
