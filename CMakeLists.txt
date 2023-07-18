cmake_minimum_required(VERSION 3.5)

# Add CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set default build type to Release
if(NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)")
endif()

# Use ccache if possible
if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
	find_program(CCACHE_FOUND "ccache")
	if(CCACHE_FOUND)
		set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_FOUND}" CACHE FILEPATH "Command to launch the C++ compiler with")
	endif()
endif()

project(LuaVis)

# Detect and enable C++14 support and disable compiler-specific extensions
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)

# Add relative include paths
include_directories("src")
include_directories("src/Shared/External")

# Add common source files
file(GLOB_RECURSE SOURCES "src/Shared/*.cpp" "src/Shared/*.c")
set(SOURCES ${SOURCES} "src/Version.cpp")

# Add client source files
file(GLOB_RECURSE CLIENT_SOURCES "src/Client/*.cpp" "src/Client/*.c")
set(SOURCES ${SOURCES} ${CLIENT_SOURCES} "src/MainClient.cpp")

# Set executable output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set executable name
set(EXECUTABLE_NAME "LuaVis")

# Disable CMake's own RPATH handling
set(CMAKE_SKIP_RPATH ON)

# Define executable
add_executable(${EXECUTABLE_NAME} WIN32 MACOSX_BUNDLE ${SOURCES})

# Detect and add SFML
find_package(SFML 2.5 COMPONENTS audio graphics window network system REQUIRED)
if(SFML_FOUND)
	include_directories(${SFML_INCLUDE_DIR})
	target_link_libraries(${EXECUTABLE_NAME} sfml-graphics sfml-window sfml-audio sfml-network sfml-system)
endif()

# Detect and add CppFS
find_package(cppfs REQUIRED)
if(cppfs_FOUND)
	include_directories(${cppfs_INCLUDE_DIRS})
	target_link_libraries(${EXECUTABLE_NAME} cppfs::cppfs)
endif()

# Detect and add OpenGL
find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIRS})
	target_link_libraries(${EXECUTABLE_NAME} OpenGL::GL)
endif()

# Detect and add zlib
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE_NAME} ZLIB::ZLIB)
endif()

# Detect and add cURL
find_package(CURL)
if(NOT DEFINED LUAVIS_ENABLE_CURL)
	set(LUAVIS_ENABLE_CURL ${CURL_FOUND} CACHE BOOL "Enable libcurl integration for HTTP downloads")
endif()

# Add cURL as an optional dependency
if(LUAVIS_ENABLE_CURL)
	if(CURL_FOUND)
		include_directories(${CURL_INCLUDE_DIRS})
		target_link_libraries(${EXECUTABLE_NAME} CURL::libcurl)
	else()
		message(FATAL_ERROR "Curl integration was requested, but libcurl could not be found")
	endif()
	set_property(SOURCE "src/Shared/Network/HTTP/CurlHTTPClient.cpp" APPEND PROPERTY COMPILE_DEFINITIONS LUAVIS_ENABLE_CURL)
endif()

# Detect and add LuaJIT
find_package(LuaJIT REQUIRED)
if(LUAJIT_FOUND)
    include_directories(${LUAJIT_INCLUDE_DIR})
    target_link_libraries(${EXECUTABLE_NAME} ${LUAJIT_LIBRARIES})
endif()

# Link Threads (preferably POSIX Threads) library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} Threads::Threads)

# Unix-specific options
if(UNIX)
	target_link_libraries(${EXECUTABLE_NAME} "-lX11")

	# Enable -rdynamic under Unix (required for LuaJIT's FFI) and specify rpath
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic -Wl,-rpath=lib")

	# Use gold linker if supported
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version OUTPUT_VARIABLE stdout ERROR_QUIET)
		if("${stdout}" MATCHES "GNU gold")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
		endif()
	endif()
endif()

# Add preprocessor macro for debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(${EXECUTABLE_NAME} PRIVATE WOS_DEBUG)
endif()

# Install
option(LUAVIS_SYMLINKS "Instead of a copy, create a symlink for the config.json file and the assets folder" OFF)

install(TARGETS LuaVis)
install(FILES run.bat;run.sh DESTINATION bin)

if(LUAVIS_SYMLINKS)
    install(CODE
        "
        if(EXISTS ${CMAKE_INSTALL_PREFIX}/bin/config.json AND NOT IS_SYMLINK ${CMAKE_INSTALL_PREFIX}/bin/config.json)
            message(FATAL_ERROR \"Cannot create symbolic link. File 'config.json' already exists.\")
        endif()
        if(EXISTS ${CMAKE_INSTALL_PREFIX}/bin/assets AND NOT IS_SYMLINK ${CMAKE_INSTALL_PREFIX}/bin/assets)
            message(FATAL_ERROR \"Cannot create symbolic link. Directory 'assets' already exists.\")
        endif()
        "
    )
    
    install(CODE "file(CREATE_LINK ${CMAKE_SOURCE_DIR}/config.json ${CMAKE_INSTALL_PREFIX}/bin/config.json SYMBOLIC)")
    install(CODE "file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets ${CMAKE_INSTALL_PREFIX}/bin/assets SYMBOLIC)")
else()
    install(CODE
        "
        if(IS_SYMLINK ${CMAKE_INSTALL_PREFIX}/bin/config.json)
            file(REMOVE ${CMAKE_INSTALL_PREFIX}/bin/config.json)
        endif()
        if(IS_SYMLINK ${CMAKE_INSTALL_PREFIX}/bin/assets)
            file(REMOVE ${CMAKE_INSTALL_PREFIX}/bin/assets)
        endif()
        "
    )

    install(FILES config.json DESTINATION bin)
    install(DIRECTORY assets/ DESTINATION bin/assets PATTERN "gfx_*" EXCLUDE PATTERN "graphs" EXCLUDE)
endif()

if(WIN32)
    set(SFML_DLL ${SFML_DIR})
    string(REPLACE "/lib/cmake/SFML" "/bin" SFML_DLL "${SFML_DLL}")
    install(DIRECTORY ${SFML_DLL}/ DESTINATION bin)

    install(FILES ${cppfs_DIR}/cppfs.dll DESTINATION bin)

    set(ZLIB_DLL ${ZLIB_LIBRARY})
    string(REPLACE "/lib" "/bin" ZLIB_DLL "${ZLIB_DLL}")
    string(REPLACE ".lib" ".dll" ZLIB_DLL "${ZLIB_DLL}")
    install(FILES ${ZLIB_DLL} DESTINATION bin)

    set(LUAJIT_DLL ${LUAJIT_INCLUDE_DIR})
    string(REPLACE "/include" "/bin" LUAJIT_DLL "${LUAJIT_DLL}")
    install(DIRECTORY ${LUAJIT_DLL}/ DESTINATION bin)
endif()
