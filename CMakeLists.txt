cmake_minimum_required(VERSION 3.5)

# Add CMake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set default build type to Release
if(NOT DEFINED CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)")
endif()

# Switch between building client and server
if(NOT DEFINED WOS_BUILD_CLIENT)
   set(WOS_BUILD_CLIENT true CACHE BOOL "Choose whether the client (true) or server (false) should be built")
endif()

# Use ccache if possible
if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
	find_program(CCACHE_FOUND "ccache")
	if(CCACHE_FOUND)
		set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_FOUND}" CACHE FILEPATH "Command to launch the C++ compiler with")
	endif()
endif()

project(WorldOfSand)

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
if(WOS_BUILD_CLIENT)
	file(GLOB_RECURSE WOS_CLIENT_SOURCES "src/Client/*.cpp" "src/Client/*.c")
	set(SOURCES ${SOURCES} ${WOS_CLIENT_SOURCES} "src/MainClient.cpp")
else()
	set(SOURCES ${SOURCES} "src/MainServer.cpp")
endif()

# Set executable output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set executable name
set(EXECUTABLE_NAME "WorldOfSand")

if(APPLE)
	# Special handling for RPATH on MacOS
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
	set(CMAKE_INSTALL_PREFIX "install")
	set(CMAKE_INSTALL_RPATH ".")
else()
	# Disable CMake's own RPATH handling
	set(CMAKE_SKIP_RPATH ON)
endif()

# Define executable
add_executable(${EXECUTABLE_NAME} WIN32 MACOSX_BUNDLE ${SOURCES})

# Define installation target
if(APPLE)
	install(TARGETS ${EXECUTABLE_NAME} BUNDLE DESTINATION ".")
endif()

# Detect and add SFML
if(WOS_BUILD_CLIENT)
	find_package(SFML 2.5 COMPONENTS audio graphics window network system REQUIRED)
	if(SFML_FOUND)
		include_directories(${SFML_INCLUDE_DIR})
		target_link_libraries(${EXECUTABLE_NAME} sfml-graphics sfml-window sfml-audio sfml-network sfml-system)
	endif()
else()
	find_package(SFML 2.5 COMPONENTS network system REQUIRED)
	if(SFML_FOUND)
		include_directories(${SFML_INCLUDE_DIR})
		target_link_libraries(${EXECUTABLE_NAME} sfml-network sfml-system)
	endif()
endif()

# Detect and add CppFS
find_package(cppfs REQUIRED)
if(cppfs_FOUND)
	include_directories(${cppfs_INCLUDE_DIRS})
	target_link_libraries(${EXECUTABLE_NAME} cppfs::cppfs)
endif()

# Detect and add OpenGL
if(WOS_BUILD_CLIENT)
	find_package(OpenGL REQUIRED)
	if(OPENGL_FOUND)
		include_directories(${OPENGL_INCLUDE_DIRS})
		if(APPLE)
			target_link_libraries(${EXECUTABLE_NAME} ${OPENGL_LIBRARIES})
		else()
			target_link_libraries(${EXECUTABLE_NAME} OpenGL::GL)
		endif()
	endif()
endif()

# Detect and add zlib
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE_NAME} ZLIB::ZLIB)
endif()

# Detect and add cURL
find_package(CURL)
if(NOT DEFINED WOS_ENABLE_CURL)
	set(WOS_ENABLE_CURL ${CURL_FOUND} CACHE BOOL "Enable libcurl integration for HTTP downloads")
endif()

# Add cURL as an optional dependency
if(WOS_ENABLE_CURL)
	if(CURL_FOUND)
		include_directories(${CURL_INCLUDE_DIRS})
		target_link_libraries(${EXECUTABLE_NAME} CURL::libcurl)
	else()
		message(FATAL_ERROR "Curl integration was requested, but libcurl could not be found")
	endif()
	set_property(SOURCE "src/Shared/Network/HTTP/CurlHTTPClient.cpp" APPEND PROPERTY COMPILE_DEFINITIONS WOS_ENABLE_CURL)
endif()

# Detect and add LuaJIT
find_package(LuaJIT REQUIRED)
if(LUAJIT_FOUND)
	if(APPLE AND "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64")
		set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
	endif()
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
	if(APPLE)
		# Enable -rdynamic under Unix (required for LuaJIT's FFI)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
	else()
		target_link_libraries(${EXECUTABLE_NAME} "-lX11")

		# Enable -rdynamic under Unix (required for LuaJIT's FFI) and specify rpath
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic -Wl,-rpath=lib")
	endif()

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

# Define project name for Version file
if(NOT DEFINED WOS_APPLICATION_NAME)
	set(WOS_APPLICATION_NAME "WorldOfSand" CACHE STRING "World of Sand application name")
endif()
if(NOT WOS_APPLICATION_NAME STREQUAL "")
	set_property(SOURCE "src/Version.cpp"
		APPEND
		PROPERTY COMPILE_DEFINITIONS
		WOS_APPLICATION_NAME=${WOS_APPLICATION_NAME}
	)
endif()

# Copy resource files
#file(COPY ${RESOURCES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
