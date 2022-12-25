cmake_minimum_required(VERSION 3.24)
project(lab32_proxy C)

set(CMAKE_C_STANDARD 11)

add_executable(lab32_proxy main.c Cache.c Cache.h TaskList.c TaskList.h write_request.c write_request.h read_request.c read_request.h read_response.c read_response.h write_response.c write_response.h threadArguments.h)
