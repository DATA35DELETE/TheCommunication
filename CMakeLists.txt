cmake_minimum_required(VERSION 3.15)
project(ToxP2PMessenger C)

set(CMAKE_C_STANDARD 11)

# MSYS2/MinGW64 için pkg-config kullan
find_package(PkgConfig REQUIRED)
pkg_check_modules(TOXCORE REQUIRED toxcore)

# pthread bul
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Executable
add_executable(tox_messenger main.c)

# Include dizinleri
target_include_directories(tox_messenger PRIVATE ${TOXCORE_INCLUDE_DIRS})

# Kütüphaneler
target_link_libraries(tox_messenger PRIVATE 
    ${TOXCORE_LIBRARIES}
    Threads::Threads
    ws2_32
    iphlpapi
)

# Compiler flags
target_compile_options(tox_messenger PRIVATE 
    -Wall 
    -Wextra
    $<$<CONFIG:Release>:-O3>
)

# MSYS2 için runtime
if(MINGW)
    target_link_options(tox_messenger PRIVATE -static-libgcc)
endif()