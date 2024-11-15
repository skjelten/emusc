cmake_minimum_required(VERSION 3.8...3.30)

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
  bar_display.cc
  bar_display.h
  emulator.cc
  emulator.h
  emusc.cc
  envelope_dialog.cc
  envelope_dialog.h
  control_rom_info_dialog.cc
  control_rom_info_dialog.h
  lcd_display.cc
  lcd_display.h
  lfo_dialog.cc
  lfo_dialog.h
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

if (Qt${QT_VERSION_MAJOR}Charts_FOUND)
  target_link_libraries(emusc-client Qt${QT_VERSION_MAJOR}::Charts)
endif()

target_link_libraries (emusc-client ${EXTERNAL_LIBRARIES})

target_compile_features(emusc-client PUBLIC cxx_std_11)
target_include_directories(emusc-client PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(emusc-client PROPERTIES CXX_EXTENSIONS OFF)

if (APPLE)
  set_property(TARGET emusc-client PROPERTY RUNTIME_OUTPUT_NAME EmuSC)
  set(MACOSX_BUNDLE_ICON_FILE emusc.icns)
  set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/../res/icons/emusc.icns")
  set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(emusc-client PRIVATE ${APP_ICON_MACOS})

  # When packing self contained bundles on macOS we need to add all dynamic
  # libraries and frameworks to the package. This is especially tricky with
  # QT based applications. QT has released a nice CMake based solution to this
  # in Qt 6.5, but that is too recent to depend on. Instead we use a hack to
  # solve this for now by using the program 'macdeployqt' (part of Qt).
  if(MACOS_BUNDLE)
    find_program(MACDEPLOYQT NAMES macdeployqt PATHS ${MACDEPLOYQT_BIN_PATH})
    if(NOT MACDEPLOYQT)
      message(FATAL_ERROR "macdeployqt not found! This application is distributed as part of the QT library and needed to create a self contained macOS bundle. Either add the application path to \$PATH or use the MACDEPLOYQT_BIN_PATH variable.")
    endif()
    install(CODE "
     execute_process(COMMAND sh -c \"${MACDEPLOYQT} ${CMAKE_CURRENT_SOURCE_DIR}/EmuSC.app\")")
  endif()

  # Using fixup_bundle does not solve all the quirks for Qt (plugins)
  #  install(CODE "
  #     include(BundleUtilities)
  #     fixup_bundle(\"${CMAKE_CURRENT_SOURCE_DIR}/EmuSC.app\" \"\" \"\")"
  #    COMPONENT bin)

elseif(WIN32)
  set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/../res/emusc-win32.rc")
  target_sources(emusc-client PRIVATE ${APP_ICON_RESOURCE_WINDOWS})

  # On Windows we need to deploy QT together with all other dependent shared
  # libraries (DLLs) to create a self contained installer package.
  # This seems to be surprisingly hard to do properly with cmake on Windows,
  # but we hack around this problem by calling a shell script in the utils
  # directory to extract the relevant files and then add them to the install.
  # Note that this hack only works for MSYS2 based installation environments in
  # Windows, i.e. Visual Studio users needs to find another solution to this
  # problem. There is also no logic to select the correct DLLs, e.g. debug vs
  # release etc. A better solution might be to use
  # $<TARGET_RUNTIME_DLLS:${TARGET}> in the future (requires CMake > 3.21).
  if(MINGW)
    # Copy all DLLs and QT plugins that emusc.exe and libemusc.dll depends on
    # in a separate directory
    add_custom_command(TARGET emusc-client POST_BUILD
      COMMAND bash -c "${CMAKE_CURRENT_SOURCE_DIR}/../utils/msys_copy_all_dlls.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../DLLs")

    # Include all DLLs and QT plugins to bin
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DLLs/" DESTINATION bin COMPONENT bin)
  endif()
endif()
