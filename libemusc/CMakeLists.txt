cmake_minimum_required(VERSION 3.12)

project(
  libemusc
  VERSION 0.2.0
  DESCRIPTION "A software library for emulating the Sound Canvas SC-55 lineup"
  HOMEPAGE_URL "https://github.com/skjelten/emusc"
  LANGUAGES CXX)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build libemusc as a shared library" ON)

add_subdirectory(src)

install(TARGETS emusc DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES src/control_rom.h src/params.h src/pcm_rom.h src/synth.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/emusc)
install(FILES AUTHORS ChangeLog COPYING COPYING.LESSER NEWS README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
