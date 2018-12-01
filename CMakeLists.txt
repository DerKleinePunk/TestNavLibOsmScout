# CMakeList.txt: CMake-Projektdatei der obersten Ebene. Führen Sie hier die globale Konfiguration aus,
# und schließen Sie Unterprojekte ein.
#
cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

SET (CMAKE_VERBOSE_MAKEFILE 1) # Use 1 for debugging, 0 for release

SET(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

if(WIN32)
	if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "where vcpkg is found")
		MESSAGE(STATUS "Using Tool chain ${CMAKE_TOOLCHAIN_FILE}")
	endif()
endif()

find_package(PkgConfig)
include(CheckCXXCompilerFlag)
include(CheckPrototypeDefinition)

MESSAGE(STATUS "VCPKG $ENV{VCPKG_ROOT}")

project ("TestNavLibOsmScout")

MESSAGE(STATUS "Try Build ${PROJECT_NAME}")
message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Building on ${CMAKE_SYSTEM_NAME}")
MESSAGE(STATUS "Build for ${TARGET}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

FIND_PACKAGE(OSMScout)

# Schließen Sie Unterprojekte ein.
add_subdirectory ("TestNavLibOsmScout")
