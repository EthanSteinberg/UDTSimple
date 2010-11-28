#CMakeLists file for server

cmake_minimum_required(VERSION 2.6)

set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wshadow")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++0x")

set(HDRS 
      )
 
set(SRCS
      src/main.cpp
      src/client.cpp
      src/clientNetwork.cpp
)
 
add_executable(UDTCli ${HDRS} ${SRCS})
target_link_libraries(UDTCli ${GTK2_LIBRARIES} ${Boost_LIBRARIES} UDT)


