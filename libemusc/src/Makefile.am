## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) 

lib_LTLIBRARIES = libemusc.la

libemusc_la_LDFLAGS = -version-info $(LIBEMUSC_SO_VERSION) -no-undefined

libemusc_la_SOURCES = \
	biquad_filter.cc	\
	control_rom.cc		\
	note.cc			\
	note_partial.cc		\
	part.cc			\
	pcm_rom.cc		\
	riaa_filter.cc		\
	synth.cc		\
	volume_envelope.cc

