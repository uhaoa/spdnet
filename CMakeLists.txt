cmake_minimum_required (VERSION 2.6)

project(spdnet)
#add_definitions(-DPLATFORM_LINUX)


if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
elseif(UNIX)
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
       set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif()

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug")
        #set(CMAKE_BUILD_TYPE "Release")
    endif()
    SET(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -ggdb -Wall -D_DEBUG")
    SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -ggdb -Wall -DNDEBUG")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fPIC")

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

file(GLOB_RECURSE ALL_SRC 
  ${PROJECT_SOURCE_DIR}/src/*.cpp 
  ${PROJECT_SOURCE_DIR}/src/*.cc 
  ${PROJECT_SOURCE_DIR}/src/*.c)

include_directories("${PROJECT_SOURCE_DIR}/src/")
add_library(spdnet SHARED ${ALL_SRC})

if(WIN32)
  target_link_libraries(spdnet ws2_32)
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(spdnet pthread)
endif()

install(TARGETS spdnet DESTINATION lib)

file(GLOB SPDNET_NET_PUBLIC_HEADERS ${PROJECT_SOURCE_DIR}/src/spdnet/net/*.h)
install (FILES ${SPDNET_NET_PUBLIC_HEADERS} DESTINATION "include/spdnet/net")

option(BUILD_EXAMPLES "build examples" ON)
if (BUILD_EXAMPLES)
     add_subdirectory(examples)
endif(BUILD_EXAMPLES)

