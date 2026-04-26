cmake_minimum_required(VERSION 3.20)

project(LightWM LANGUAGES C)

if (NOT WIN32)
	message(FATAL_ERROR "LightWM can only be built on Windows.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build types" FORCE)

if (MSVC)
	set(CMAKE_C_FLAGS_DEBUG "/DDEBUG /Zi /W3" CACHE STRING "Debug flags" FORCE)
	set(CMAKE_C_FLAGS_RELEASE "/Ox /W3" CACHE STRING "Release flags" FORCE)
endif()

set(lightwmExeSources
	wm.c
	tiling.c
	error.c
	keyboard.c
	shared_mem.c
)

set(lightwmDllSources
	wm_dll.c
	error.c
	shared_mem.c
)

add_executable(lightwm WIN32 ${lightwmExeSources})
add_library(lightwm_dll SHARED ${lightwmDllSources})

target_link_libraries(lightwm PRIVATE dwmapi gdi32 kernel32 shell32 user32)
target_link_libraries(lightwm_dll PRIVATE kernel32 user32)

if (MSVC)
	target_link_options(lightwm PRIVATE
		/ENTRY:wWinMain
		/SUBSYSTEM:WINDOWS
	)

	target_link_options(lightwm_dll PRIVATE
		/ENTRY:DllMain
		/SUBSYSTEM:WINDOWS
	)
endif()

set_target_properties(lightwm PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
)

set_target_properties(lightwm_dll PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
)
