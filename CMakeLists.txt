cmake_minimum_required(VERSION 3.20)

project(SnapWM LANGUAGES C)

if (NOT WIN32)
	message(FATAL_ERROR "SnapWM can only be built on Windows.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Supported build types" FORCE)

if (MSVC)
	set(CMAKE_C_FLAGS_DEBUG "/DDEBUG /Zi /W3" CACHE STRING "Debug flags" FORCE)
	set(CMAKE_C_FLAGS_RELEASE "/Ox /W3" CACHE STRING "Release flags" FORCE)
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Zi /W3" CACHE STRING "RelWithDebInfo flags" FORCE)
endif()

set(snapwmExeSources
	src/app/wm.c
	src/app/tiling.c
	src/app/keyboard.c
	src/common/error.c
	src/common/shared_mem.c
)

set(snapwmDllSources
	src/dll/wm_dll.c
	src/common/error.c
	src/common/shared_mem.c
)

add_executable(snapwm WIN32 ${snapwmExeSources})
add_library(snapwm_dll SHARED ${snapwmDllSources})

target_include_directories(snapwm PRIVATE src/app src/common)
target_include_directories(snapwm_dll PRIVATE src/common)

target_link_libraries(snapwm PRIVATE dwmapi gdi32 kernel32 shell32 user32)
target_link_libraries(snapwm_dll PRIVATE kernel32 user32)

if (MSVC)
	target_link_options(snapwm PRIVATE
		/ENTRY:wWinMain
		/SUBSYSTEM:WINDOWS
	)

	target_link_options(snapwm_dll PRIVATE
		/ENTRY:DllMain
		/SUBSYSTEM:WINDOWS
	)
endif()

set_target_properties(snapwm PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/relwithdebinfo"
	PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/relwithdebinfo"
)

set_target_properties(snapwm_dll PROPERTIES
	OUTPUT_NAME snapwm
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/relwithdebinfo"
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/relwithdebinfo"
	PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/debug"
	PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/release"
	PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/relwithdebinfo"
)
