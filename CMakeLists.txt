cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

foreach(p
    CMP0053 # CMake 3.1
    )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()


set(META_PROJECT_NAME           "GeohazardVis")
set(META_PROJECT_DESCRIPTION    "")
set(META_AUTHOR_ORGANIZATION    "")
set(META_AUTHOR_DOMAIN          "https://github.com/kateyy/geohazardvis")
set(META_AUTHOR_MAINTAINER      "karsten.tausche@student.hpi.de")
set(META_VERSION_MAJOR          "0")
set(META_VERSION_MINOR          "0")
set(META_VERSION_PATCH          "0")
set(META_VERSION_REVISION       "${GIT_REV}")
set(META_VERSION                "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}")
set(META_NAME_VERSION           "${META_PROJECT_NAME} v${META_VERSION} (${META_VERSION_REVISION})")


option(OPTION_ADD_CPPCHECK_TARGETS "Add a cppcheck target per C++-target." OFF)
option(OPTION_BUILD_TESTS "Build tests (if gtest is found)." ON)
option(OPTION_BUILD_TOOLS "" ON)
option(OPTION_IDE_SPLIT_HEADERS_SOURCES "List header and source files in different IDE folders." ON)


set(CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo;RelNoOptimization")
set(CMAKE_CXX_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_SHARED_LINKER_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_EXE_LINKER_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif()


# Generate folders for IDE targets (e.g., VisualStudio solutions)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${META_PROJECT_NAME})



project(${META_PROJECT_NAME} C CXX)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CMakeDependentOption)
include(cmake/Custom.cmake)
include(cmake/GetGitRevisionDescription.cmake)
include(cmake/GitDate.cmake)

# Get git revision
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING "${GIT_SHA1}" 0 12 GIT_REV)
get_git_commit_date(${GIT_SHA1} GIT_DATE)
set(GIT_REVISION_FILE ${CMAKE_BINARY_DIR}/revision)
set(VERSION_INFO_FILE ${CMAKE_CURRENT_BINARY_DIR}/vcs_commit_info.h)
configure_file(cmake/vcs_commit_info.h.in ${VERSION_INFO_FILE})


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

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMPILER_IS_CLANG YES)
else()
    set(COMPILER_IS_CLANG NO)
endif()

# Setup platform specifics (compile flags, etc., ...)
if(MSVC)
    if(COMPILER_IS_CLANG)
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformWindowsMSVCClang.cmake)
    else()
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformWindowsMSVC.cmake)
    endif()
elseif(UNIX)
    if (COMPILER_IS_CLANG)
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformLinuxClang.cmake)
    else()
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformLinuxGCC.cmake)
    endif()
else()
    message(WARNING "Unsupported platform/compiler combination")
endif()



#
# Deployment/installation setup
#

set(project ${META_PROJECT_NAME})
if(WIN32)
    set(INSTALL_ROOT ".")                       # C:\Programme\<project>
    set(INSTALL_DATA "data")                     # C:\Programme\<project>
    set(INSTALL_BIN ".")                      # C:\Programme\<project>
    set(INSTALL_SHARED ".")                     # C:\Programme\<project>
    set(INSTALL_LIB "lib")                      # C:\Programme\<project>\lib
    set(INSTALL_INCLUDE "include")              # C:\Programme\<project>\include
    set(INSTALL_DOC "doc")                      # C:\Programme\<project>\doc
    set(INSTALL_SHORTCUTS ".")                  # Not available under Windows
    set(INSTALL_ICONS ".")                      # Not available under Windows
    set(INSTALL_INIT ".")                       # Not available under Windows
    set(INSTALL_PLUGINS_BIN "${INSTALL_BIN}/plugins")
    set(INSTALL_PLUGINS_SHARED "${INSTALL_SHARED}/plugins")
    set(INSTALL_PLUGINS_LIB "${INSTALL_LIB}/plugins")

else()
#     set(INSTALL_ROOT "share/${project}")        # /usr/[local]/share/<project>
#     set(INSTALL_DATA "share/${project}")        # /usr/[local]/share/<project>
    set(INSTALL_ROOT ".")
    set(INSTALL_DATA "data")
    set(INSTALL_BIN "bin")                      # /usr/[local]/bin
    set(INSTALL_SHARED "lib")                   # /usr/[local]/lib
    set(INSTALL_LIB "lib")                      # /usr/[local]/lib
    set(INSTALL_INCLUDE "include")              # /usr/[local]/include
    set(INSTALL_DOC "share/doc/${project}")     # /usr/[local]/share/doc/<project>
    set(INSTALL_SHORTCUTS "share/applications") # /usr/[local]/share/applications
    set(INSTALL_ICONS "share/pixmaps")          # /usr/[local]/share/pixmaps
    set(INSTALL_INIT "/etc/init")               # /etc/init (upstart init scripts)
    set(INSTALL_PLUGINS_BIN "${INSTALL_BIN}/plugins")
    set(INSTALL_PLUGINS_SHARED "${INSTALL_SHARED}/plugins")
    set(INSTALL_PLUGINS_LIB "${INSTALL_LIB}/plugins")

    install(PROGRAMS packages/${META_PROJECT_NAME}.sh
        DESTINATION ${INSTALL_ROOT})
endif()

option(OPTION_INSTALL_3RDPARTY_BINARIES "Include 3rdparty libraries (libzeug, VTK, Qt) in installations and packages." ON)

# Install the project meta files
#install(FILES geohazardvis-config.cmake DESTINATION ${INSTALL_ROOT})
#install(FILES AUTHORS DESTINATION               ${INSTALL_ROOT})
#install(FILES LICENSE DESTINATION               ${INSTALL_ROOT})


# Find / add third-party components

set(THIRD_PARTY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty")
set(THIRD_PARTY_BUILD_DIR "${PROJECT_BINARY_DIR}/ThirdParty")
include(ExternalProject)

find_package(OpenGL REQUIRED)

set(VTK_COMPONENTS
    vtkGUISupportQtOpenGL   # QVTKWidget2
    vtkRenderingAnnotation  # vtkCubeAxesActor, vtkScalarBarActor
    vtkFiltersTexture
    vtkInteractionWidgets
    vtkIOXML
    vtkViewsContext2D
    vtkChartsCore
)

find_package(VTK COMPONENTS ${VTK_COMPONENTS})

set(VTK_VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
set(VTK_REQUIRED_VERSION 6.3)

if(VTK_VERSION VERSION_LESS VTK_REQUIRED_VERSION)
    message(FATAL_ERROR "VTK version ${VTK_VERSION} was found but version 6.3 or newer is required.")
endif()

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

if(VTK_RENDERING_BACKEND_VERSION EQUAL 2)
    list(APPEND VTK_COMPONENTS vtkRenderingLICOpenGL2)
    list(APPEND VTK_COMPONENTS vtkRenderingContextOpenGL2)
else()
    list(APPEND VTK_COMPONENTS vtkRenderingLIC)
endif()



if(VTK_VERSION VERSION_LESS 7.1)
    set(_vtkIOExport_module_name "vtkIOExport")
    set(_vtkRenderingGL2PS_module_name "vtkRenderingGL2PS")
else()
    # OpenGL context based PostScript export is now optional
    set(_vtkIOExport_module_name "vtkIOExport${VTK_RENDERING_BACKEND}")
    if(VTK_RENDERING_BACKEND_VERSION EQUAL 1)
        set(_vtkRenderingGL2PS_module_name "vtkRenderingGL2PS")
    else()
        set(_vtkRenderingGL2PS_module_name "vtkRenderingGL2PSOpenGL2")
    endif()
endif()

if(TARGET ${_vtkIOExport_module_name} AND TARGET ${_vtkRenderingGL2PS_module_name})
    set(VTK_has_GLExport2PS 1)
    list(APPEND VTK_COMPONENTS ${_vtkIOExport_module_name} ${_vtkRenderingGL2PS_module_name})
else()
    set(VTK_has_GLExport2PS 0)
    message("VTK was not build with ${_vtkIOExport_module_name} and ${_vtkRenderingGL2PS_module_name}, PostScript/EPS export will not be available.")
endif()


# find additional VTK components
find_package(VTK COMPONENTS ${VTK_COMPONENTS})


include_directories(SYSTEM ${VTK_INCLUDE_DIRS})
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})

set(PROJECT_QT_COMPONENTS Core Gui Widgets OpenGL)
if (UNIX)
    # required for platform plugin deployment
    list(APPEND PROJECT_QT_COMPONENTS DBus Svg)
endif()
find_package(Qt5 5.3 COMPONENTS ${PROJECT_QT_COMPONENTS})
if(NOT ${Qt5Core_VERSION} VERSION_LESS 5.5)
    find_package(Qt5 COMPONENTS Concurrent)
    list(APPEND PROJECT_QT_COMPONENTS Concurrent)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(SYSTEM # disable warnings produced by Qt headers (GCC)
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
)


option(OPTION_USE_SYSTEM_LIBZEUG "Search for installed libzeug libraries instead of compiling them" OFF)
CMAKE_DEPENDENT_CACHEVARIABLE(OPTION_LIBZEUG_GIT_REPOSITORY "https://github.com/kateyy/libzeug.git" STRING
    "URL/path of the libzeug git repository" "NOT OPTION_USE_SYSTEM_LIBZEUG" OFF)

if(OPTION_USE_SYSTEM_LIBZEUG)
    find_package(libzeug REQUIRED)
else()
    set(LIBZEUG_FOUND 1)
    set(LIBZEUG_PREFIX "${THIRD_PARTY_BUILD_DIR}/libzeug")
    set(libzeug_DIR "${LIBZEUG_PREFIX}/install")

    ExternalProject_Add(libzeug
        PREFIX ${LIBZEUG_PREFIX}
        SOURCE_DIR "${THIRD_PARTY_SOURCE_DIR}/libzeug"
        GIT_REPOSITORY ${OPTION_LIBZEUG_GIT_REPOSITORY}
        GIT_TAG "geohazardvis_2015_09"
        BUILD_COMMAND ${CMAKE_COMMAND}
            -DBUILD_CONFIG:STRING=${CMAKE_CFG_INTDIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/libzeugExternalProjectBuild.cmake
        INSTALL_COMMAND ""
        CMAKE_CACHE_ARGS
            -DOPTION_BUILD_TESTS:bool=OFF
            -DOPTION_USE_OPENMP:bool=OFF
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
        )
    else()
        set(LIBZEUG_LIBRARIES
            "${libzeug_DIR}/lib/libsignalzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libloggingzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libreflectionzeug$<$<CONFIG:Debug>:d>.so"
            "${libzeug_DIR}/lib/libpropertyguizeug$<$<CONFIG:Debug>:d>.so"
        )
    endif()
endif()


if(OPTION_INSTALL_3RDPARTY_BINARIES)
    include(cmake/DeploySystemLibraries.cmake)

    if (VTK_BUILD_SHARED_LIBS)
        set(_vtkDeployFiles)
        if (WIN32)
            # Windows: VTK build tree contains separated folders for each configuration
            foreach(_libName ${VTK_LIBRARIES})
                list(APPEND _vtkDeployFiles "${VTK_DIR}/bin/$<$<STREQUAL:$<CONFIG>,RelNoOptimization>:RelWithDebInfo>$<$<NOT:$<STREQUAL:$<CONFIG>,RelNoOptimization>>:$<CONFIG>>/${_libName}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
            endforeach()
        elseif(UNIX)
            # Linux: configuration type of installed VTK must be the same as the deployment configuration for this project.
            # We have to rely on this correct setup here.
            foreach(_libName ${VTK_LIBRARIES})
                get_filename_component(_realPath "lib${_libName}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.so" REALPATH
                    BASE_DIR ${VTK_INSTALL_PREFIX}/lib)
                list(APPEND _vtkDeployFiles ${_realPath})
            endforeach()
        endif()
        install(FILES ${_vtkDeployFiles} DESTINATION ${INSTALL_SHARED})
    endif()

    if (WIN32)
        install(FILES
            "${libzeug_DIR}/bin/signalzeug$<$<CONFIG:Debug>:d>.dll"
            "${libzeug_DIR}/bin/loggingzeug$<$<CONFIG:Debug>:d>.dll"
            "${libzeug_DIR}/bin/reflectionzeug$<$<CONFIG:Debug>:d>.dll"
            "${libzeug_DIR}/bin/propertyguizeug$<$<CONFIG:Debug>:d>.dll"
            DESTINATION ${INSTALL_SHARED}
        )
    elseif(UNIX)
        if (NOT OPTION_USE_SYSTEM_LIBZEUG)
            install(FILES ${LIBZEUG_LIBRARIES} DESTINATION ${INSTALL_SHARED})
        else()
            install(FILES
                ${LIBZEUG_SIGNAL_LIBRARY_DEBUG}
                ${LIBZEUG_LOGGING_LIBRARY_DEBUG}
                ${LIBZEUG_REFLECTION_LIBRARY_DEBUG}
                ${LIBZEUG_PROPERTYGUI_LIBRARY_DEBUG}
                DESTINATION ${INSTALL_SHARED}
                CONFIGURATIONS Debug
            )
            install(FILES
                ${LIBZEUG_SIGNAL_LIBRARY_RELEASE}
                ${LIBZEUG_LOGGING_LIBRARY_RELEASE}
                ${LIBZEUG_REFLECTION_LIBRARY_RELEASE}
                ${LIBZEUG_PROPERTYGUI_LIBRARY_RELEASE}
                DESTINATION ${INSTALL_SHARED}
                CONFIGURATIONS Release RelWithDebInfo RelNoOptimization
            )
        endif()
    endif()
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(cmake/config.h.in ${CONFIG_FILE})

generate_cppcheck_suppressions()


set(IDE_FOLDER "")
add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("data")

# Plugins that require additional third party libraries may add their required runtime paths
# using the addPluginRuntimPathEntries function.
# Executable targets depend on that, to find required DLLs at runtime
include(cmake/ProjectConfigSetup.cmake)
set(IDE_FOLDER "Plugins")
add_subdirectory("plugins")

set(IDE_FOLDER "")
add_subdirectory("app")
set(IDE_FOLDER "Tests")
add_subdirectory("tests")

set(IDE_FOLDER "Deployment, Documentation")
add_subdirectory("docs")
add_subdirectory("packages")

if (OPTION_BUILD_TOOLS)
    set(IDE_FOLDER "Tools")
    add_subdirectory("tools")
endif()
