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


option(OPTION_BUILD_TESTS "Build tests (if gtest is found)." ON)
option(OPTION_BUILD_TOOLS "" ON)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


# Generate folders for IDE targets (e.g., VisualStudio solutions)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(OPTION_IDE_SPLIT_HEADERS_SOURCES "List header and source files in different IDE folders." ON)

# Include custom cmake functions
include(cmake/Custom.cmake)
include(cmake/GitRevision.cmake)
include(cmake/ProjectConfigSetup.cmake)

include(CMakeDependentOption)

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

if(WIN32)
    option(CPACK_INSTALL_3RDPARTY_DLLS "Include 3rdparty dlls (VTK, Qt) in packages." ON)
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
install(FILES ${CMAKE_BINARY_DIR}/revision DESTINATION ${INSTALL_ROOT})


# Find / add third-party components

set(THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/ThirdParty")
include(ExternalProject)

find_package(OpenGL REQUIRED)

set(VTK_COMPONENTS
    vtkGUISupportQt         # QVTKWidget
    vtkRenderingAnnotation  # vtkCubeAxesActor, vtkScalarBarActor
    vtkFiltersTexture
    vtkInteractionWidgets
    vtkIOXML
    vtkViewsContext2D
    vtkChartsCore
)

find_package(VTK 6.3 REQUIRED ${VTK_COMPONENTS})
# replaced by explicit calls below (http://www.kitware.com/source/home/post/116)
#include(${VTK_USE_FILE})

if(NOT "${VTK_QT_VERSION}" STREQUAL "5")
    message(FATAL_ERROR "VTK was built with Qt version ${VTK_QT_VERSION}, but Qt5 is required.")
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


# configuration dependent VTK components

if (${VTK_RENDERING_BACKEND_VERSION} EQUAL 2)
    list(APPEND VTK_COMPONENTS vtkRenderingLICOpenGL2)
    list(APPEND VTK_COMPONENTS vtkRenderingContextOpenGL2)
else()
    list(APPEND VTK_COMPONENTS vtkRenderingLIC)
endif()


if(TARGET vtkIOExport AND TARGET vtkRenderingGL2PS)
    set(VTK_has_GLExport2PS 1)
    list(APPEND VTK_COMPONENTS vtkIOExport vtkRenderingGL2PS)
else()
    set(VTK_has_GLExport2PS 0)
    message("VTK was not build with vtkIOExport and vtkRenderingGL2PS, PostScript/EPS export will not be available.")
endif()

# find additional VTK components
find_package(VTK REQUIRED ${VTK_COMPONENTS})


include_directories(SYSTEM ${VTK_INCLUDE_DIRS})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})


set(FindQt5_Components Core Gui Widgets)
if(NOT ${Qt5Core_VERSION} VERSION_LESS 5.5)
    list(APPEND FindQt5_Components Concurrent)
endif()



option(OPTION_USE_SYSTEM_LIBZEUG "Search for installed Libzeug Libraries instead of compiling them" OFF)
if(OPTION_USE_SYSTEM_LIBZEUG)
    find_package(libzeug REQUIRED)
else()
    set(LIBZEUG_FOUND 1)
    set(LIBZEUG_PREFIX "${THIRD_PARTY_DIR}/libzeug")
    set(libzeug_DIR "${LIBZEUG_PREFIX}/install")

    list(APPEND FindQt5_Components OpenGL)

    ExternalProject_Add(libzeug
        PREFIX ${LIBZEUG_PREFIX}
        GIT_REPOSITORY "https://github.com/kateyy/libzeug.git"
        GIT_TAG "geohazardvis_2015_09"
        CMAKE_CACHE_ARGS
            -DOPTION_BUILD_TESTS:bool=OFF
            -DQt5Core_DIR:path=${Qt5Core_DIR}
            -DQt5Gui_DIR:path=${Qt5Gui_DIR}
            -DQt5OpenGL_DIR:path=${Qt5OpenGL_DIR}
            -DQt5Widgets_DIR:path=${Qt5Widgets_DIR}
            -DCMAKE_INSTALL_PREFIX:path=${libzeug_DIR}
            -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
    )

    set(LIBZEUG_INCLUDES
        "${libzeug_DIR}/include"
    )

    if(WIN32)
        set(LIBZEUG_LIBRARIES
            "${libzeug_DIR}/lib/signalzeug$<$<CONFIG:Debug>:d>.lib"
            "${libzeug_DIR}/lib/loggingzeug$<$<CONFIG:Debug>:d>.lib"
            "${libzeug_DIR}/lib/reflectionzeug$<$<CONFIG:Debug>:d>.lib"
            "${libzeug_DIR}/lib/propertyguizeug$<$<CONFIG:Debug>:d>.lib"
            "${libzeug_DIR}/lib/threadingzeug$<$<CONFIG:Debug>:d>.lib"
        )
    else()
        set(LIBZEUG_LIBRARIES
            "${libzeug_DIR}/lib/libsignalzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libloggingzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libreflectionzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libpropertyguizeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libthreadingzeug$<$<CONFIG:Debug>:d>.so"
        )
    endif()
endif()


find_package(Qt5 COMPONENTS ${FindQt5_Components})


set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(SYSTEM # disable warnings produced by Qt headers (GCC)
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
)


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
        "${libzeug_DIR}/bin/propertyguizeug$<$<CONFIG:Debug>:d>.dll"
        "${libzeug_DIR}/bin/threadingzeug$<$<CONFIG:Debug>:d>.dll"
    )

    install(FILES ${DEPLOY_FILES} DESTINATION ${INSTALL_BIN})
endif()


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(IDE_FOLDER "")
add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("app")

set(IDE_FOLDER "Deployment, Documentation")
add_subdirectory("docs")
add_subdirectory("packages")

set(IDE_FOLDER "Tests")
add_subdirectory("tests")

if (OPTION_BUILD_TOOLS)
    set(IDE_FOLDER "Tools")
    add_subdirectory("tools")
endif()
