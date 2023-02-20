# Find JACK Audio Connection Kit development files for EmuSC
# https://jackaudio.org/

find_path(JACK_INCLUDE_DIR
    NAMES
        jack/jack.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
)

find_library(JACK_libjack_LIBRARY
    NAMES
        libjack${EXTENSION} jack
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
)

set(JACK_LIBRARIES ${JACK_libjack_LIBRARY})
set(JACK_LIBRARY ${JACK_LIBRARIES})
set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(JACK  DEFAULT_MSG
                                  JACK_LIBRARY JACK_INCLUDE_DIR)
