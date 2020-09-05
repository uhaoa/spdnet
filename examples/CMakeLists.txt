include_directories("${PROJECT_SOURCE_DIR}/src/")

#include_directories("/usr/local/ssl/include")

if (WIN32)
set(LINK_LIBS "ws2_32")
elseif (UNIX)
set(LINK_LIBS "pthread")
endif ()

if(SPDNET_USE_OPENSSL)
	set(LINK_LIBS "${LINK_LIBS} -lssl -lcrypto -ldl")
endif(SPDNET_USE_OPENSSL)

if (UNIX)
    find_package(Threads REQUIRED)
endif (UNIX)

add_executable(pingpongclient pingpongclient.cpp)
target_link_libraries(pingpongclient ${LINK_LIBS})

add_executable(pingpongserver pingpongserver.cpp)
target_link_libraries(pingpongserver ${LINK_LIBS})

add_executable(ttcp_client ttcp_client.cpp)
target_link_libraries(ttcp_client ${LINK_LIBS})

add_executable(ttcp_server ttcp_server.cpp)
target_link_libraries(ttcp_server ${LINK_LIBS})

add_executable(httpclient httpclient.cpp)
target_link_libraries(httpclient ${LINK_LIBS})

add_executable(httpserver httpserver.cpp)
target_link_libraries(httpserver -ldl ${LINK_LIBS})

