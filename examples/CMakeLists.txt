enable_testing()

find_package(Threads REQUIRED)

add_executable(pingpongclient pingpongclient.cpp)
if(WIN32)
	target_link_libraries(pingpongclient spdnet)
elseif(UNIX)
	target_link_libraries(pingpongclient pthread spdnet)
endif()

add_executable(pingpongserver pingpongserver.cpp)
if(WIN32)
	target_link_libraries(pingpongserver spdnet)
elseif(UNIX)
	target_link_libraries(pingpongserver pthread spdnet)
endif()


add_executable(ttcp_client ttcp_client.cpp)
if(WIN32)
	target_link_libraries(ttcp_client spdnet)
elseif(UNIX)
	target_link_libraries(ttcp_client pthread spdnet)
endif()


add_executable(ttcp_server ttcp_server.cpp)
if(WIN32)
	target_link_libraries(ttcp_server spdnet)
elseif(UNIX)
	target_link_libraries(ttcp_server pthread spdnet)
endif()


