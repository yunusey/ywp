cmake_minimum_required(VERSION 3.14)
project(ywp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.c")

add_subdirectory(external/wlrlayer)
add_subdirectory(external/cava)

option(ENABLE_ASAN "Enable AddressSanitizer" ON)

add_executable(
	ywp
	${SOURCES}
)


target_include_directories(
	ywp
	PRIVATE
	external/cavacore
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
