cmake_minimum_required(VERSION 3.12...3.30)

project(
  libemusc
  VERSION 0.3.0
  DESCRIPTION "A software library for emulating the Sound Canvas SC-55 lineup"
  HOMEPAGE_URL "https://github.com/skjelten/emusc"
  LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build libemusc as a shared library" ON)

add_subdirectory(src)

# TODO: Find a better way to handle pkg-config file generation
#       Challenge is that the correct path is first known in the install-phase
configure_file("cmake/emusc.pc.in" "emusc.pc" @ONLY)

include(GNUInstallDirs)

install(TARGETS emusc DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)
install(FILES AUTHORS ChangeLog COPYING COPYING.LESSER NEWS README.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT lib)

install(FILES src/control_rom.h src/params.h src/pcm_rom.h src/synth.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/emusc COMPONENT dev)
install(FILES emusc.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT dev)

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_BINARY_DIR)
  include(CPack)
endif()
