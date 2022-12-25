cmake_minimum_required(VERSION 3.0.0)
project(dflow_calc VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)

include(CTest)
enable_testing()

add_executable(dflow_calc dflow_calc.cpp dflow_calc.h dflow_main.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)