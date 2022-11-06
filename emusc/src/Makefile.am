## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) 

bin_PROGRAMS = emusc

emusc_SOURCES =				\
	audio_config_dialog.cc		\
	audio_output.cc			\
	audio_output_alsa.cc		\
	audio_output_core.cc		\
	audio_output_null.cc		\
	audio_output_pulse.cc		\
	audio_output_qt.cc		\
	audio_output_win32.cc		\
	emulator.cc			\
	emusc.cc			\
	control_rom_info_dialog.cc	\
	main_window.cc			\
	midi_config_dialog.cc		\
	midi_input.cc			\
	midi_input_alsa.cc		\
	midi_input_core.cc		\
	midi_input_keyboard.cc		\
	midi_input_win32.cc		\
	rom_config_dialog.cc		\
	scene.cc

nodist_emusc_SOURCES =			\
	main_window.moc			\
	audio_config_dialog.moc		\
	audio_output_qt.moc		\
	control_rom_info_dialog.moc	\
	midi_config_dialog.moc		\
	rom_config_dialog.moc		\
	scene.moc			\
	emulator.moc			\
	qrc_emusc.cc

BUILT_SOURCES =				\
	main_window.moc			\
	audio_config_dialog.moc		\
	audio_output_qt.moc		\
	control_rom_info_dialog.moc	\
	midi_config_dialog.moc		\
	rom_config_dialog.moc		\
	scene.moc			\
	emulator.moc			\
	qrc_emusc.cc

CLEANFILES =				\
	main_window.moc			\
	audio_config_dialog.moc		\
	audio_output_qt.moc		\
	control_rom_info_dialog.moc	\
	midi_config_dialog.moc		\
	rom_config_dialog.moc		\
	scene.moc			\
	emulator.moc			\
	qrc_emusc.cc

qrc_emusc.cc: Makefile
	$(QT_RCC) -name emusc ../resources/emusc.qrc -o $@

main_window.moc: main_window.h
	$(QT_MOC) -i $< -o $@

audio_config_dialog.moc: audio_config_dialog.h
	$(QT_MOC) -i $< -o $@

audio_output_qt.moc: audio_output_qt.h
	$(QT_MOC) -i $< -o $@

control_rom_info_dialog.moc: control_rom_info_dialog.h
	$(QT_MOC) -i $< -o $@

midi_config_dialog.moc: midi_config_dialog.h
	$(QT_MOC) -i $< -o $@

rom_config_dialog.moc: rom_config_dialog.h
	$(QT_MOC) -i $< -o $@

scene.moc: scene.h
	$(QT_MOC) -i $< -o $@

emulator.moc: emulator.h
	$(QT_MOC) -i $< -o $@
