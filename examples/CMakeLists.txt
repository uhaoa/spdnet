enable_testing()

find_package(Threads REQUIRED)

add_executable(pingpongclient pingpongclient.cpp)
target_link_libraries(pingpongclient pthread spdnet)

add_executable(pingpongserver pingpongserver.cpp)
target_link_libraries(pingpongserver pthread spdnet)

add_executable(ttcp_client ttcp_client.cpp)
target_link_libraries(ttcp_client pthread spdnet)

add_executable(ttcp_server ttcp_server.cpp)
target_link_libraries(ttcp_server pthread spdnet)

