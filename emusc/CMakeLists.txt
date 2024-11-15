cmake_minimum_required(VERSION 3.12...3.30)

project(
  emusc-client
  VERSION 0.2.0
  HOMEPAGE_URL "https://github.com/skjelten/emusc"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(USE_MACOS_BUNDLE "no")

set(ALSA_AUDIO  "no")
set(CORE_AUDIO  "no")
set(JACK_AUDIO  "no")
set(PULSE_AUDIO "no")
set(QT_AUDIO    "no")
set(WAV_AUDIO   "no")
set(WIN32_AUDIO "no")

set(ALSA_MIDI  "no")
set(CORE_MIDI  "no")
set(WIN32_MIDI "no")

set(USE_QTCHARTS "no")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} QUIET OPTIONAL_COMPONENTS Multimedia Charts)

if (Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
  add_definitions(-D__QT_AUDIO__)
  set(QT_AUDIO "yes")
endif()

if (Qt${QT_VERSION_MAJOR}Charts_FOUND)
  add_definitions(-D__USE_QTCHARTS__)
  set(USE_QTCHARTS "yes")
endif()

# TODO: Find libemusc with option to look for system or local

if (UNIX AND NOT APPLE)
  find_package(ALSA QUIET)
  if (ALSA_FOUND)
    list(APPEND EXTERNAL_INCLUDE_FILES ${ALSA_INCLUDE_DIRS})
    list(APPEND EXTERNAL_LIBRARIES ${ALSA_LIBRARIES})
    add_definitions(-D__ALSA_AUDIO__ -D__ALSA_MIDI__)
    set(ALSA_AUDIO "yes")
    set(ALSA_MIDI "yes")
  endif(ALSA_FOUND)

  find_package(PulseAudio QUIET)
  if (PULSEAUDIO_FOUND)
    list(APPEND EXTERNAL_INCLUDE_FILES ${PULSEAUDIO_INCLUDE_DIR})
    list(APPEND EXTERNAL_LIBRARIES ${PULSEAUDIO_LIBRARY})
    add_definitions(-D__PULSE_AUDIO__)
    set(PULSE_AUDIO "yes")
  endif()

  find_package(JACK QUIET)
  if (JACK_FOUND)
    list(APPEND EXTERNAL_INCLUDE_FILES ${JACK_INCLUDE_DIR})
    list(APPEND EXTERNAL_LIBRARIES ${JACK_LIBRARY})
    add_definitions(-D__JACK_AUDIO__)
    set(JACK_AUDIO "yes")
  endif()

# TODO: Check for libraries actually being available
elseif(WIN32)
  list(APPEND EXTERNAL_LIBRARIES winmm)
  add_definitions(-D__WIN32_AUDIO__ -D__WIN32_MIDI__)
  set(WIN32_AUDIO "yes")
  set(WIN32_MIDI "yes")

#  target_link_libraries(${target_name} winmm.lib)

elseif(APPLE)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  if (COREFOUNDATION_LIBRARY)
    list(APPEND EXTERNAL_LIBRARIES ${COREFOUNDATION_LIBRARY})
  else()
    message(ERROR "CoreFoundation framework not found, but is needed on macOS")
  endif()

  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  if (COREAUDIO_LIBRARY AND AUDIOUNIT_LIBRARY)
    list(APPEND EXTERNAL_LIBRARIES ${COREAUDIO_LIBRARY};${AUDIOUNIT_LIBRARY})
    add_definitions(-D__CORE_AUDIO__)
    set(CORE_AUDIO "yes")
  else()
    message(WARNING "CoreAudio framework not found, no audio output possible ")
  endif()

find_library(COREMIDI_LIBRARY CoreMIDI)
  if (COREMIDI_LIBRARY)
    list(APPEND EXTERNAL_LIBRARIES ${COREMIDI_LIBRARY})
    add_definitions(-D__CORE_MIDI__)
    set(CORE_MIDI "yes")
  else()
    message(WARNING "CoreMIDI framework not found, no external MIDI events")
  endif()

endif()

# Hack to make CPACK_DEBIAN_BIN_PACKAGE_SHLIBDEPS work properly (CMAKE < 3.20)
#set(CMAKE_INSTALL_RPATH "../lib/usr/lib")
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(src)

if(APPLE)
  if(MACOS_BUNDLE)
    set_target_properties(emusc-client PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
      MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
    set(USE_MACOS_BUNDLE "yes")
  else()
    set_target_properties(emusc-client PROPERTIES MACOSX_BUNDLE FALSE)
  endif()

elseif(WIN32)
  option(BUILD_WIN32_EXECUTABLE "Build executable without debug console" ON)
  set_target_properties(emusc-client PROPERTIES WIN32_EXECUTABLE ${BUILD_WIN32_EXECUTABLE})
endif()

# Currently all targets are expected to support WAV output
set(WAV_AUDIO "yes")
target_compile_definitions(emusc-client PUBLIC __WAV_AUDIO__)

include(GNUInstallDirs)

install(TARGETS emusc-client
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
  BUNDLE DESTINATION . COMPONENT bin)

if(NOT APPLE AND NOT MACOS_BUNDLE)
  install(FILES AUTHORS ChangeLog COPYING NEWS README.md
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/emusc COMPONENT bin)
endif()

if(UNIX AND NOT APPLE)
  install(FILES res/emusc.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications COMPONENT bin)
  foreach(EMUSC_ICON_RESOLUTION 16x16;32x32;128x128;256x256;512x512)
    install(FILES res/icons/${EMUSC_ICON_RESOLUTION}/emusc.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${EMUSC_ICON_RESOLUTION}/apps COMPONENT bin)
  endforeach(EMUSC_ICON_RESOLUTION)
endif(UNIX AND NOT APPLE)

message("\n"
        " EmuSC client summary:\n"
        "-=====================-\n")

if(APPLE)
  message(" Platform: macOS\n"
          "  * Build bundle: ${USE_MACOS_BUNDLE}")
elseif(UNIX)
  message(" Platform: UNIX")
elseif(WIN32)
  message(" Platform: Windows\n"
          "  * Debug console: ${WIN32_EXECUTABLE}")
else()
  message(" Platform: unkown")
endif()

message("\n"
        " Qt version: Qt${QT_VERSION_MAJOR}\n"
        " QtCharts: ${USE_QTCHARTS}\n"
        "\n"
        " Audio APIs:\n"
        "  * ALSA  : ${ALSA_AUDIO}\n"
        "  * Core  : ${CORE_AUDIO}\n"
        "  * Jack  : ${JACK_AUDIO}\n"
        "  * Null  : yes\n"
        "  * Pulse : ${PULSE_AUDIO}\n"
        "  * Qt    : ${QT_AUDIO}\n"
        "  * WAV   : ${WAV_AUDIO}\n"
        "  * Win32 : ${WIN32_AUDIO}\n"
        "\n"
        " MIDI APIs:\n"
        "  * ALSA  : ${ALSA_MIDI}\n"
        "  * Core  : ${CORE_MIDI}\n"
        "  * Win32 : ${WIN32_MIDI}\n"
        "\n"
        " Make sure all needed APIs are found before you continue\n"
        "\n")

# CPack support
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

if(APPLE AND MACOS_BUNDLE)
  set(CPACK_PACKAGE_NAME "EmuSC")
  set(CPACK_DMG_VOLUME_NAME "EmuSC")
  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/res/images/installers/dmg_background.png")
  set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/utils/CMake_DMG_setup.scpt")
endif()

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_BINARY_DIR)
  include(CPack)
endif()
