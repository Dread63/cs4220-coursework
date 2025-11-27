cmake_minimum_required(VERSION 3.10)
project(project3 C)

set(CMAKE_C_STANDARD 11)

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Server executable
add_executable(server server.c)
target_link_libraries(server OpenSSL::SSL OpenSSL::Crypto)

# Client executable
add_executable(client client.c)
target_link_libraries(client OpenSSL::SSL OpenSSL::Crypto)
