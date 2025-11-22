cmake_minimum_required(VERSION 3.13)
project(cava C)

set(CAVA_SOURCES
    cavacore.c
    input_methods.c
    input/common.c
)

option(CAVA_INPUT_FIFO  "Use FIFO input backend" OFF)
option(CAVA_INPUT_PULSE "Use PulseAudio backend" ON)
option(CAVA_INPUT_ALSA  "Use ALSA backend" OFF)

# Collect backend sources FIRST (no compile definitions yet)
if(CAVA_INPUT_FIFO)
    list(APPEND CAVA_SOURCES input/fifo.c)
endif()

if(CAVA_INPUT_PULSE)
    list(APPEND CAVA_SOURCES input/pulse.c)
endif()

if(CAVA_INPUT_ALSA)
    list(APPEND CAVA_SOURCES input/alsa.c)
endif()

# Create the library
add_library(cava STATIC ${CAVA_SOURCES})

# Now attach the compile definitions to the cava target
if(CAVA_INPUT_FIFO)
    target_compile_definitions(cava PUBLIC INPUT_AUDIO_METHOD=INPUT_FIFO)
endif()

if(CAVA_INPUT_PULSE)
    target_compile_definitions(cava PUBLIC INPUT_AUDIO_METHOD=INPUT_PULSE)
endif()

if(CAVA_INPUT_ALSA)
    target_compile_definitions(cava PUBLIC INPUT_AUDIO_METHOD=INPUT_ALSA)
endif()

# Link libs AFTER creating the target
find_library(LIBM m)
find_library(FFTW3 fftw3)
find_library(PTHREAD pthread)

target_link_libraries(cava PRIVATE
    ${LIBM}
    ${FFTW3}
    ${PTHREAD}
)

# Includes
target_include_directories(cava PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/input
)

# PulseAudio link
if(CAVA_INPUT_PULSE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSEAUDIO REQUIRED libpulse-simple)

    list(APPEND CAVA_SOURCES input/pulse.c)

    target_include_directories(cava PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
    target_link_libraries(cava PRIVATE ${PULSEAUDIO_LIBRARIES})
    target_compile_options(cava PRIVATE ${PULSEAUDIO_CFLAGS_OTHER})
endif()

# ALSA link
if(CAVA_INPUT_ALSA)
	find_package(ALSA REQUIRED)
	target_link_libraries(cava PRIVATE ALSA::ALSA)
endif()

