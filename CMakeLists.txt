# Minimum CMake required
cmake_minimum_required(VERSION 3.1.3)

set(CMAKE_CXX_COMPILER "/usr/local/bin/g++")


if(protobuf_VERBOSE)
  message(STATUS "Protocol Buffers Configuring...")
endif()

# CMake policies
cmake_policy(SET CMP0022 NEW)
# On MacOS use @rpath/ for target's install name prefix path
if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif ()
# Clear VERSION variables when no VERSION is given to project()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

# Project
project(protobuf_test C CXX)

set(BOOST_ROOT  /usr/local/lib/boost/include)
set(Boost_LIBRARY_DIRS /usr/local/lib/boost/lib)
link_directories(${PCL_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})    


find_package(Boost COMPONENTS regex system REQUIRED)


# Add c++11 flags
if (CYGWIN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
else()
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_executable(protoRecv protoRecv.cpp package.pb.cc)
target_link_libraries(protoRecv pthread crypto boost_system boost_thread libpthread.so.0 protobuf ${Boost_LIBRARIES})

add_executable(protoSend protoSend.cpp package.pb.cc)
target_link_libraries(protoSend pthread crypto libpthread.so.0 protobuf ${Boost_LIBRARIES})

