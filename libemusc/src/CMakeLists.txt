cmake_minimum_required(VERSION 3.8...3.24)

configure_file(config.h.in config.h)

add_library(emusc
  ahdsr.cc
  ahdsr.h
  biquad_filter.cc
  biquad_filter.h
  control_rom.cc
  control_rom.h
  lowpass_filter.cc
  lowpass_filter.h
  note.cc
  note.h
  params.h
  part.cc
  part.h
  partial.cc
  partial.h
  pcm_rom.cc
  pcm_rom.h
  riaa_filter.cc
  riaa_filter.h
  settings.cc
  settings.h
  synth.cc
  synth.h
  tva.cc
  tva.h
  tvf.cc
  tvf.h
  tvp.cc
  tvp.h
  wavetable.cc
  wavetable.h)


target_compile_features(emusc PUBLIC cxx_std_11)
target_include_directories(emusc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(emusc PROPERTIES CXX_EXTENSIONS OFF)
