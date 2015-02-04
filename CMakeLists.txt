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

set(IDE_SPLIT_HEADERS_SOURCES ON CACHE BOOL "List header and source files in different IDE folders.")

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
    set(INSTALL_DATA ".")                     # C:\Programme\<project>
    set(INSTALL_BIN ".")                      # C:\Programme\<project>
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

set(CPACK_INSTALL_3RDPARTY_DLLS TRUE CACHE BOOL "Include 3rdparty dlls (VTK, Qt) in packages.")

# Install the project meta files
#install(FILES geohazardvis-config.cmake DESTINATION ${INSTALL_ROOT})
#install(FILES AUTHORS DESTINATION               ${INSTALL_ROOT})
install(FILES LICENSE DESTINATION               ${INSTALL_ROOT})

# Install the data directory including the data files it contains.
install(DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION ${INSTALL_DATA})

# Add a revision file containing the git-head tag for cpack and install. The function
# "create_revision_file (...)" is defined in cmake/GitRevision.cmake
create_revision_file(${CMAKE_BINARY_DIR}/revision ${INSTALL_ROOT})
install(FILES ${CMAKE_BINARY_DIR}/revision DESTINATION ${INSTALL_ROOT})


set(VTK_COMPONENTS 
    vtkGUISupportQtOpenGL   # QVTKWidget2
    vtkRenderingAnnotation  # vtkCubeAxesActor, vtkScalarBarActor
    vtkInteractionWidgets
    vtkIOXML
    vtkViewsContext2D
    vtkChartsCore
)

find_package(VTK REQUIRED ${VTK_COMPONENTS})
# replaced by explicit calls below (http://www.kitware.com/source/home/post/116)
#include(${VTK_USE_FILE})

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


if(TARGET vtkIOExport AND TARGET vtkRenderingGL2PS)
    set(VTK_has_GLExport2PS 1)
    find_package(VTK COMPONENTS ${VTK_COMPONENTS} vtkIOExport vtkRenderingGL2PS)
else()
    set(VTK_has_GLExport2PS 0)
    message("VTK was not build with vtkIOExport and vtkRenderingGL2PS, PostScript/EPS export will not be available.")
endif()


include_directories(${VTK_INCLUDE_DIRS})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})


find_package(Qt5 5.2.1 COMPONENTS Widgets)
find_package(libzeug REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32 AND CPACK_INSTALL_3RDPARTY_DLLS)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_BIN})
    include(InstallRequiredSystemLibraries)

    foreach(LIB ${VTK_LIBRARIES})
        list(APPEND DEPLOY_FILES "${VTK_DIR}/bin/$<CONFIG>/${LIB}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
    endforeach()

    list(APPEND DEPLOY_FILES 
        "${libzeug_DIR}/bin/signalzeug$<$<CONFIG:Debug>:d>.dll"
        "${libzeug_DIR}/bin/loggingzeug$<$<CONFIG:Debug>:d>.dll"
        "${libzeug_DIR}/bin/reflectionzeug$<$<CONFIG:Debug>:d>.dll"
        "${libzeug_DIR}/bin/propertyguizeug$<$<CONFIG:Debug>:d>.dll")
    
    install(FILES ${DEPLOY_FILES} DESTINATION ${INSTALL_BIN})
endif()


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
