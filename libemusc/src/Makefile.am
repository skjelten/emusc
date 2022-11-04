## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) 

lib_LTLIBRARIES = libemusc.la

libemusc_la_LDFLAGS = -version-info $(LIBEMUSC_SO_VERSION) -no-undefined

libemusc_la_SOURCES =		\
	biquad_filter.cc	\
	biquad_filter.h		\
	control_rom.cc		\
	control_rom.h		\
	note.cc			\
	note.h			\
	note_partial.cc		\
	note_partial.h		\
	part.cc			\
	part.h			\
	pcm_rom.cc		\
	pcm_rom.h		\
	riaa_filter.cc		\
	riaa_filter.h		\
	synth.cc		\
	synth.h			\
	volume_envelope.cc	\
	volume_envelope.h

extincludedir = $(includedir)/emusc
extinclude_HEADERS =	\
	synth.h		\
	control_rom.h	\
	pcm_rom.h
