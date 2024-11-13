cmake_minimum_required(VERSION 3.8...3.30)

configure_file(config.h.in config.h)

add_library(emusc
  ahdsr.cc
  ahdsr.h
  allpass_filter.cc
  allpass_filter.h
  biquad_filter.cc
  biquad_filter.h
  chorus.cc
  chorus.h
  comb_filter.cc
  comb_filter.h
  control_rom.cc
  control_rom.h
  delay.cc
  delay.h
  highpass_filter2.cc
  highpass_filter2.h
  lowpass_filter1.cc
  lowpass_filter1.h
  lowpass_filter2.cc
  lowpass_filter2.h
  note.cc
  note.h
  params.h
  part.cc
  part.h
  partial.cc
  partial.h
  pcm_rom.cc
  pcm_rom.h
  resample.cc
  resample.h
  reverb.cc
  reverb.h
  settings.cc
  settings.h
  synth.cc
  synth.h
  system_effects.cc
  system_effects.h
  tva.cc
  tva.h
  tvf.cc
  tvf.h
  tvp.cc
  tvp.h
  wave_generator.cc
  wave_generator.h)


target_compile_features(emusc PUBLIC cxx_std_11)
target_include_directories(emusc PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(emusc PROPERTIES CXX_EXTENSIONS OFF VERSION ${CMAKE_PROJECT_VERSION} SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})
