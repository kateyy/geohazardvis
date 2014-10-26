cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(META_PROJECT_NAME "GeohazardVis")
set(META_PROJECT_DESCRIPTION "")
set(META_VERSION_MAJOR "0")
set(META_VERSION_MINOR "0")
set(META_VERSION_PATCH "0")
set(META_VERSION "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}")
set(META_AUTHOR_ORGANIZATION "")
set(META_AUTHOR_DOMAIN "https://github.com/kateyy/geohazardvis")
set(META_AUTHOR_MAINTAINER "karsten.tausche@student.hpi.de")


project (${META_PROJECT_NAME} C CXX)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


# Generate folders for IDE targets (e.g., VisualStudio solutions)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include custom cmake functions
include(cmake/Custom.cmake)
include(cmake/GitRevision.cmake)


# PLATFORM AND ARCHITECTURE

# Architecture (32/64 bit)
set(X64 OFF)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64 ON)
endif()


set(DEFAULT_COMPILE_DEFS
    $<$<CONFIG:Debug>:_DEBUG>            # Debug build
    $<$<CONFIG:Release>:NDEBUG>          # optimized builds
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# Setup platform specifics (compile flags, etc., ...)
if(MSVC)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformWindowsMSVC.cmake)
elseif(UNIX)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformLinuxGCC.cmake)
else()
    message(WARNING "Unsupported platform/compiler combination")
endif()



#
# Deployment/installation setup
#

set(project ${META_PROJECT_NAME})
if(WIN32)
    set(INSTALL_ROOT ".")                       # C:\Programme\<project>
    set(INSTALL_DATA "bin")                     # C:\Programme\<project>
    set(INSTALL_BIN "bin")                      # C:\Programme\<project>
    set(INSTALL_SHARED ".")                     # C:\Programme\<project>
    set(INSTALL_LIB "lib")                      # C:\Programme\<project>\lib
    set(INSTALL_INCLUDE "include")              # C:\Programme\<project>\include
    set(INSTALL_DOC "doc")                      # C:\Programme\<project>\doc
    set(INSTALL_SHORTCUTS ".")                  # Not available under Windows
    set(INSTALL_ICONS ".")                      # Not available under Windows
    set(INSTALL_INIT ".")                       # Not available under Windows

else()
    set(INSTALL_ROOT "share/${project}")        # /usr/[local]/share/<project>
    set(INSTALL_DATA "share/${project}")        # /usr/[local]/share/<project>
    set(INSTALL_BIN "bin")                      # /usr/[local]/bin
    set(INSTALL_SHARED "lib")                   # /usr/[local]/lib
    set(INSTALL_LIB "lib")                      # /usr/[local]/lib
    set(INSTALL_INCLUDE "include")              # /usr/[local]/include
    set(INSTALL_DOC "share/doc/${project}")     # /usr/[local]/share/doc/<project>
    set(INSTALL_SHORTCUTS "share/applications") # /usr/[local]/share/applications
    set(INSTALL_ICONS "share/pixmaps")          # /usr/[local]/share/pixmaps
    set(INSTALL_INIT "/etc/init")               # /etc/init (upstart init scripts)
endif()

# Install the project meta files
#install(FILES geohazardvis-config.cmake DESTINATION ${INSTALL_ROOT})
#install(FILES AUTHORS DESTINATION               ${INSTALL_ROOT})
install(FILES LICENSE DESTINATION               ${INSTALL_ROOT})

# Install the data directory including the data files it contains.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION ${INSTALL_DATA})

# Add a revision file containing the git-head tag for cpack and install. The function
# "create_revision_file (...)" is defined in cmake/GitRevision.cmake
create_revision_file(${CMAKE_BINARY_DIR}/revision ${INSTALL_ROOT})


find_package(VTK REQUIRED
    vtkGUISupportQt         # QVTKWidget
    vtkRenderingAnnotation  # vtkCubeAxesActor, vtkScalarBarActor
    vtkInteractionWidgets
)
include(${VTK_USE_FILE})

if (NOT ${VTK_VERSION_MAJOR} STREQUAL "6")
    message(FATAL_ERROR "VTK 6 is required! Found ${VTK_VERSION_MAJOR}")
endif()

if("${VTK_QT_VERSION}" STREQUAL "")
    message(FATAL_ERROR "VTK was not built with Qt")
elseif(NOT "${VTK_QT_VERSION}" STREQUAL "5")
    message(FATAL_ERROR "VTK was a wrong Qt version (${VTK_QT_VERSION}). Only Qt5 is supported.")
endif()

if ("${VTK_RENDERING_BACKEND}" STREQUAL "")
    set(VTK_RENDERING_BACKEND "OpenGL")
endif()
if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL" OR "${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
    message(STATUS "VTK Rendering Backend: " ${VTK_RENDERING_BACKEND})
    if (${VTK_RENDERING_BACKEND} STREQUAL "OpenGL")
        set(VTK_RENDERING_BACKEND_VERSION 1)
    else()
        set(VTK_RENDERING_BACKEND_VERSION 2)
    endif()
else()
    message(FATAL_ERROR "Unsupported VTK Rendering Backend: ${VTK_RENDERING_BACKEND}")
endif()


find_package(Qt5Widgets REQUIRED)
find_package(libzeug REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("app")
add_subdirectory("docs")
add_subdirectory(packages)
