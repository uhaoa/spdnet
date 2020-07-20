include_directories("${PROJECT_SOURCE_DIR}/src/")

if (UNIX)
    find_package(Threads REQUIRED)
endif (UNIX)

add_executable(pingpongclient pingpongclient.cpp)
if (WIN32)
    target_link_libraries(pingpongclient ws2_32)
elseif (UNIX)
    target_link_libraries(pingpongclient pthread)
endif ()

add_executable(pingpongserver pingpongserver.cpp)
if (WIN32)
    target_link_libraries(pingpongserver ws2_32)
elseif (UNIX)
    target_link_libraries(pingpongserver pthread)
endif ()


add_executable(ttcp_client ttcp_client.cpp)
if (WIN32)
    target_link_libraries(ttcp_client ws2_32)
elseif (UNIX)
    target_link_libraries(ttcp_client pthread)
endif ()


add_executable(ttcp_server ttcp_server.cpp)
if (WIN32)
    target_link_libraries(ttcp_server ws2_32)
elseif (UNIX)
    target_link_libraries(ttcp_server pthread)
endif ()

add_executable(httpserver httpserver.cpp)
if (WIN32)
    target_link_libraries(httpserver ws2_32)
elseif (UNIX)
    target_link_libraries(httpserver pthread)
endif ()


