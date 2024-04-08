cmake_minimum_required(VERSION 3.7...3.24)

project(
  emusc
  VERSION 0.0.1
  LANGUAGES CXX)
  
option(emusc_WITH_EMUSC_CLIENT "Build GUI client application" TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(libemusc)

if (emusc_WITH_EMUSC_CLIENT)
  add_subdirectory(emusc)
endif()
