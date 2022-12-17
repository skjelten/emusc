cmake_minimum_required(VERSION 3.7...3.24)

project(
  emusc
  VERSION 0.0.1
  LANGUAGES CXX)
  
option(emusc_WITH_EMUSC_CLIENT "Build GUI client application" TRUE)

add_subdirectory(libemusc)

if (emusc_WITH_EMUSC_CLIENT)
  add_subdirectory(emusc)
endif()
