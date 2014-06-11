cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set (META_PROJECT_NAME "GeohazardVis")
set (META_VERSION_MAJOR "0")
set (META_VERSION_MINOR "0")
set (META_VERSION_PATCH "0")
set (META_VERSION "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}")
set (META_AUTHOR_ORGANIZATION "")
set (META_AUTHOR_DOMAIN "https://github.com/kateyy/geohazardvis")

project (${META_PROJECT_NAME} C CXX)

option(OPTION_LOCAL_INSTALL "Install to local directory instead of system" OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "Limited Configs" FORCE)



set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH 
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


include(cmake/Custom.cmake)


# PLATFORM AND ARCHITECTURE

# Architecture (32/64 bit)
set(X64 OFF)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64 ON)
endif()

# Check for linux
if(UNIX AND NOT APPLE)
    set(LINUX 1)
endif()
    
    
# Setup platform specifics (compile flags, etc., ...)
if(MSVC)
    message(STATUS "Configuring for platform Windows/MSVC.")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformWindowsMSVC.cmake)
elseif(LINUX AND CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Configuring for platform Linux/GCC.")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformLinuxGCC.cmake)
else()
    message(WARNING "Unsupported platform/compiler combination")
endif()


# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


find_package(VTK REQUIRED
    vtkGUISupportQtOpenGL   # QVTKWidget
    vtkRenderingAnnotation  # vtkCubeAxesActor
)
include(${VTK_USE_FILE})

if("${VTK_QT_VERSION}" STREQUAL "")
    message(FATAL_ERROR "VTK was not built with Qt")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(libzeug REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("app")
