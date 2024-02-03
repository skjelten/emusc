cmake_minimum_required(VERSION 3.8...3.24)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Copy include files from libemusc (needed due to directory include path)
configure_file(../../libemusc/src/control_rom.h include/emusc/control_rom.h COPYONLY)
configure_file(../../libemusc/src/params.h include/emusc/params.h COPYONLY)
configure_file(../../libemusc/src/pcm_rom.h include/emusc/pcm_rom.h COPYONLY)
configure_file(../../libemusc/src/synth.h include/emusc/synth.h COPYONLY)
include_directories(include)

configure_file(config.h.in config.h)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libemusc/src)

add_executable(emusc-client
  audio_output.cc
  audio_output.h
  audio_output_alsa.cc
  audio_output_alsa.h
  audio_output_core.cc
  audio_output_core.h
  audio_output_jack.cc
  audio_output_jack.h
  audio_output_null.cc
  audio_output_null.h
  audio_output_pulse.cc
  audio_output_pulse.h
  audio_output_qt.cc
  audio_output_qt.h
  audio_output_wav.cc
  audio_output_wav.h
  audio_output_win32.cc
  audio_output_win32.h
  emulator.cc
  emulator.h
  emusc.cc
  control_rom_info_dialog.cc
  control_rom_info_dialog.h
  main_window.cc
  main_window.h
  midi_input.cc
  midi_input.h
  midi_input_alsa.cc
  midi_input_alsa.h
  midi_input_core.cc
  midi_input_core.h
  midi_input_win32.cc
  midi_input_win32.h
  preferences_dialog.cc
  preferences_dialog.h
  rom_info.cc
  rom_info.h
  scene.cc
  scene.h
  synth_dialog.cc
  synth_dialog.h
  ../res/emusc.qrc)

set_property(TARGET emusc-client PROPERTY RUNTIME_OUTPUT_NAME emusc)

include_directories(${EXTERNAL_INCLUDE_FILES})


target_link_libraries(emusc-client emusc)

target_link_libraries(emusc-client Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(emusc-client Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(emusc-client Qt${QT_VERSION_MAJOR}::Gui)

if (Qt${QT_VERSION_MAJOR}Multimedia_FOUND)
  target_link_libraries(emusc-client Qt${QT_VERSION_MAJOR}::Multimedia)
endif()

target_link_libraries (emusc-client ${EXTERNAL_LIBRARIES})

target_compile_features(emusc-client PUBLIC cxx_std_11)
target_include_directories(emusc-client PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(emusc-client PROPERTIES CXX_EXTENSIONS OFF)
