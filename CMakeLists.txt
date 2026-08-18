cmake_minimum_required(VERSION 4.0)
project(GameServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF )

find_package(OpenSSL REQUIRED)
include_directories(include ${OPENSSL_INCLUDE_DIR})

add_executable(proxy src/main.cpp
        ../Library/Caches/JetBrains/CLion2025.2/.docker/2025_2/Docker2muchfun/proxy-dev-env_latest/usr/lib/gcc/aarch64-linux-gnu/15/include/UDPSocket.h
        src/UDPSocket.cpp
        UDPSocket.h
        ../Library/Caches/JetBrains/CLion2025.2/.docker/2025_2/Docker2muchfun/proxy-dev-env_latest/usr/lib/gcc/aarch64-linux-gnu/15/include/LoadBalancer.h
        src/LoadBalancer.cpp
        LoadBalancer.h
        AuditSigner.h
        src/AuditSigner.cpp
)

target_link_libraries(proxy PRIVATE OpenSSL::Crypto OpenSSL::SSL ${OPENSSL_LIBRARIES})