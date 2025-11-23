cmake_minimum_required(VERSION 3.14)
project(ywp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.c")

add_subdirectory(external/wlrlayer)
add_subdirectory(external/cava)

option(ENABLE_ASAN "Enable AddressSanitizer" ON)

# We need to embed shaders
find_program(XXD_EXE xxd REQUIRED)
file(GLOB_RECURSE SHADERS "${SHADERS_DIR}/*.vert" "${SHADERS_DIR}/*.frag")
set(SHADER_HEADERS "")
foreach(SHADER_FILE ${SHADERS})
	file(RELATIVE_PATH SHADER_REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders" ${SHADER_FILE})
	set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_REL_PATH}.h")

	get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
	file(MAKE_DIRECTORY ${HEADER_DIR})
	string(REGEX REPLACE "[/.]" "_" VAR_NAME "shaders_${SHADER_REL_PATH}")

	add_custom_command(
		OUTPUT  ${HEADER_FILE}
		COMMAND ${XXD_EXE} -i -n ${VAR_NAME} ${SHADER_FILE} ${HEADER_FILE}
		# This is the C variable name xxd will use
		COMMAND ${CMAKE_COMMAND} -E rename ${HEADER_FILE} ${HEADER_FILE} # This is a trick to update timestamp
		# This tells CMake the .h file depends on the shader file.
		# If you edit the shader, CMake will re-run xxd.
		MAIN_DEPENDENCY ${SHADER_FILE}
		COMMENT "Embedding shader: ${SHADER_REL_PATH}"
	)

	list(APPEND SHADER_HEADERS ${HEADER_FILE})
endforeach()

add_executable(
	ywp
	${SOURCES}
	${SHADER_HEADERS}
)

target_include_directories(
	ywp
	PRIVATE
	external/cavacore
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)

find_library(LIBM m)
find_library(GL GL)
find_library(GLESV2 GLESv2)
find_library(EGL EGL)
find_library(WAYLAND_CLIENT wayland-client)
find_library(WAYLAND_EGL wayland-egl)
find_library(XKBCOMMON xkbcommon)

target_link_libraries(
	ywp
	PRIVATE
	${GL}
	${LIBM}
	${GLESV2}
	${EGL}
	${WAYLAND_CLIENT}
	${WAYLAND_EGL}
	${XKBCOMMON}
	wlrlayer
	cava
)

if(ENABLE_ASAN)
	target_compile_options(ywp PRIVATE -fsanitize=address -fno-omit-frame-pointer)
	target_link_options(ywp PRIVATE -fsanitize=address)
endif()

install(TARGETS ywp DESTINATION bin)
