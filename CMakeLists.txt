cmake_minimum_required(VERSION 2.6)

project(GtkApp)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -std=c++0x")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arch)

set(Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0" "1.41.0" "1.41" "1.40.0" "1.40" "1.39.0" "1.39" "1.38.0" "1.38" "1.37.0" "1.37" )
set(BOOST_COMPONENTS system thread date_time filesystem)
find_package(Boost COMPONENTS ${BOOST_COMPONENTS} QUIET)
include_directories(${Boost_INCLUDE_DIR})

find_package(GTK2 2.6 REQUIRED gtk)
include_directories(${GTK2_INCLUDE_DIRS})

add_library(UDT STATIC IMPORTED)
set_target_properties(UDT PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libudt.a)
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(client) 
add_subdirectory(server)
