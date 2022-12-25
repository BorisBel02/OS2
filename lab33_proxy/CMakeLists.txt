cmake_minimum_required(VERSION 3.24)
project(lab33_proxy C)

set (CFLAGS="-fno-omit-frame-pointer -fsanitize=address -g")
set (LDFLAGS="-fsanitize=address -g" ./configure)
set(CMAKE_C_STANDARD 11)

add_executable(lab33_proxy main.c TaskList.c TaskList.h workThread.c workThread.h Cache.c Cache.h)
