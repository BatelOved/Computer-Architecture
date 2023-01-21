cmake_minimum_required(VERSION 3.0.0)
project(bp VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)

include(CTest)
enable_testing()

add_executable(sim_main main.c sim_api.c sim_api.h core_api.cpp core_api.h)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
