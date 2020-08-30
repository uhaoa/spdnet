cmake_minimum_required(VERSION 2.6)

project(spdnet)

find_library(OPENSSL NAMES ssl crypto)
if(OPENSSL)
add_definitions(-DSPDNET_USE_OPENSSL)
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
elseif (UNIX)
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif ()

    if (NOT CMAKE_BUILD_TYPE)
	#set(CMAKE_BUILD_TYPE "Debug")
        set(CMAKE_BUILD_TYPE "Release")
    endif ()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb -Wall -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -ggdb -Wall -DNDEBUG")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif ()


SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
install(DIRECTORY src/spdnet
        DESTINATION "${INCLUDE_INSTALL_DIR}")


option(BUILD_EXAMPLES "build examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif (BUILD_EXAMPLES)

