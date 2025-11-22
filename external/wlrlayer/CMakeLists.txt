cmake_minimum_required(VERSION 3.4...3.28 FATAL_ERROR)
project(wlrlayer C)

add_library(wlrlayer)

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
if (NOT WAYLAND_SCANNER_EXECUTABLE)
    message(FATAL_ERROR "Failed to find wayland-scanner")
endif()

# Copied from [external/glfw/src/CMakeLists.txt](https://github.com/raysan5/raylib/blob/5b88e4fe5f69253acf3a2cdf77a84e167d08777d/src/external/glfw/src/CMakeLists.txt#L79)
macro(generate_wayland_protocol protocol_file)
	set(protocol_path "${CMAKE_CURRENT_SOURCE_DIR}/${protocol_file}")

	string(REGEX REPLACE "\\.xml$" "-client-protocol.h" header_file ${protocol_file})
	string(REGEX REPLACE "\\.xml$" "-client-protocol.c" code_file ${protocol_file})

	add_custom_command(OUTPUT ${header_file}
		COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header "${protocol_path}" ${header_file}
		DEPENDS "${protocol_path}"
		VERBATIM)

	add_custom_command(OUTPUT ${code_file}
		COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code "${protocol_path}" ${code_file}
		DEPENDS "${protocol_path}"
		VERBATIM)

	target_sources(wlrlayer PRIVATE ${header_file} ${code_file})
endmacro()

generate_wayland_protocol("wlr-layer-shell.xml")
generate_wayland_protocol("xdg-shell.xml")

target_include_directories(wlrlayer
	PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

include(CheckFunctionExists)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)
if (HAVE_MEMFD_CREATE)
    target_compile_definitions(wlrlayer PRIVATE HAVE_MEMFD_CREATE)
endif()

find_library(WAYLAND_CLIENT_LIBRARY NAMES wayland-client)
find_library(WAYLAND_CURSOR NAMES wayland-cursor)
find_library(XKBCOMMON NAMES xkbcommon)
if (WAYLAND_CLIENT_LIBRARY)
	target_link_libraries(wlrlayer PRIVATE ${WAYLAND_CURSOR} ${WAYLAND_CLIENT_LIBRARY} ${XKBCOMMON})
else()
    message(WARNING "libwayland-client not found. Linking will likely fail.")
endif()
