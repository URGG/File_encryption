cmake_minimum_required(VERSION 3.16)
project(GameServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1. Find all required system packages first
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(include ${OPENSSL_INCLUDE_DIR})

# 2. Create the executable using ONLY local relative paths
# Do not include absolute cache paths here. The compiler will find
# the headers automatically if they are in the same directory as the .cpp files.
add_executable(proxy
        src/main.cpp
        src/UDPSocket.cpp
        src/LoadBalancer.cpp
        src/AuditSigner.cpp
        src/BloomFilter.cpp
)

# 3. Link libraries to the executable AFTER it has been created
target_link_libraries(proxy PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)