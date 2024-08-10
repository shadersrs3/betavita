cmake_minimum_required(VERSION 3.6)
project(betavita)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "/usr/bin/clang")
set(CMAKE_C_COMPILER "/usr/bin/clang++")

add_compile_options(-Wall -fjump-tables -O3)

set(SOURCE_FILES
    src/loader/self_loader.cpp
    src/memory/memory.cpp
    src/logger/logger.cpp
    src/main.cpp

    src/hle/hle.cpp
    src/hle/kernel.cpp
    src/hle/modules/SceLibKernel.cpp
    src/hle/modules/SceSysmem.cpp
)

set(HEADER_FILES
    include/loader/self_loader.h
    include/loader/ELF.h
    include/memory/memory.h
    include/logger/logger.h

    include/hle/hle.h
    include/hle/kernel.h
    include/hle/modules/SceLibKernel.h
    include/hle/modules/SceSysmem.h
)

set(HEADER_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../ext/unicorn-master/include)

include_directories(SYSTEM ${HEADER_DIRECTORIES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${HEADER_DIRECTORIES})
target_link_libraries(${PROJECT_NAME}
    /usr/lib/x86_64-linux-gnu/libstdc++.so.6
    /usr/lib/x86_64-linux-gnu/libz.so
    ${PROJECT_SOURCE_DIR}/../ext/unicorn-master/build/libunicorn.so)
