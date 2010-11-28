#CMakeLists file for server

cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wshadow")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++0x")

set(HDRS 
      )
 
set(SRCS
      src/main.cpp
      src/server.cpp
      src/serverNetwork.cpp
)
 
add_executable(UDTSer ${HDRS} ${SRCS})
target_link_libraries(UDTSer ${GTK2_LIBRARIES} ${Boost_LIBRARIES} UDT)


