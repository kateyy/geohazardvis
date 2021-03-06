cmake_minimum_required(VERSION 3.3)

foreach(p
    CMP0065 # CMake 3.4
    CMP0071 # CMake 3.10
    )
  if(POLICY ${p})
    cmake_policy(SET ${p} NEW)
  endif()
endforeach()


set(META_PROJECT_NAME           "GeohazardVis")
set(META_PROJECT_DESCRIPTION    "")
set(META_AUTHOR_ORGANIZATION    "")
set(META_AUTHOR_DOMAIN          "https://github.com/kateyy/geohazardvis")
set(META_AUTHOR_MAINTAINER      "Karsten Tausche")
set(META_AUTHOR_MAINTAINER_EMAIL "geodev@posteo.org")
set(META_VERSION_MAJOR          "0")
set(META_VERSION_MINOR          "2")
set(META_VERSION_PATCH          "2")
set(META_VERSION_REVISION       "${GIT_REV}")
set(META_VERSION                "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}")
set(META_NAME_VERSION           "${META_PROJECT_NAME} v${META_VERSION} (${META_VERSION_REVISION})")


option(OPTION_ADD_CPPCHECK_TARGETS "Add a cppcheck target per C++-target." ON)
option(OPTION_BUILD_TESTS "Build tests (if gtest is found)." ON)
option(OPTION_BUILD_TOOLS "" ON)
option(OPTION_IDE_SPLIT_HEADERS_SOURCES "List header and source files in different IDE folders." ON)
option(OPTION_INSTALL_3RDPARTY_BINARIES "Include 3rdparty libraries (libzeug, VTK, Qt) in installations and packages." ON)

option(OPTION_ENABLE_LIC2D "Integrate Line Integral Convolution representation for displacement volume slices." OFF)
option(OPTION_ENABLE_TEXTURING "Enable texturing support for polygonal meshes." OFF)

option(OPTION_USE_QVTKOPENGLWIDGET "Use QVTKOpenGLWidget instead of the older QVTKWidget2 to integrate VTK OpenGL contexts into the UI" ON)

# Generate folders for IDE targets (e.g., VisualStudio solutions)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${META_PROJECT_NAME})


set(ACCEPTED_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo;RelNoOptimization")
# This doesn't seem to have an effect: RelNoOptimization is still linked to Debug VTK libraries.
# If there is a property way to set this, remove VTK target property hacks in FindThirdParties.cmake
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug")
set(CMAKE_CXX_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_SHARED_LINKER_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_EXE_LINKER_FLAGS_RELNOOPTIMIZATION "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


# if generatorIsMultiConfig is not detected, CMAKE_CONFIGURATION_TYPES must not be set!
if ((NOT DEFINED generatorIsMultiConfig AND DEFINED CMAKE_CONFIGURATION_TYPES)
        OR (generatorIsMultiConfig AND (NOT "${CMAKE_CONFIGURATION_TYPES}" STREQUAL "${ACCEPTED_CONFIGURATION_TYPES}")))
    message(WARNING "Manually setting CMAKE_CONFIGURATION_TYPES is not supported in this project.")
    unset(CMAKE_CONFIGURATION_TYPES CACHE)
    set(_resetBuildType TRUE)
endif()


project(${META_PROJECT_NAME}
    VERSION ${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}.0
    LANGUAGES C CXX)


# project() sets CMAKE_CONFIGURATION_TYPES only for multi-configuration generators.
if (NOT DEFINED generatorIsMultiConfig)
    if (DEFINED CMAKE_CONFIGURATION_TYPES)
        set(generatorIsMultiConfig YES CACHE INTERNAL "")
    else()
        set(generatorIsMultiConfig NO CACHE INTERNAL "")
    endif()
    set(_resetBuildType TRUE)
endif()

# reject invalid build types to prevent unexpected build behavior
if (NOT generatorIsMultiConfig AND CMAKE_BUILD_TYPE
    AND NOT CMAKE_BUILD_TYPE IN_LIST ACCEPTED_CONFIGURATION_TYPES)
    message(WARNING "Rejecting invalid CMAKE_BUILD_TYPE: \"${CMAKE_BUILD_TYPE}\"")
    unset(CMAKE_BUILD_TYPE CACHE)
    set(_resetBuildType TRUE)
endif()

# Initialize CMAKE_BUILD_TYPE or CMAKE_CONFIGURATION_TYPES depending on current generator type
if (_resetBuildType)
    if (generatorIsMultiConfig)
        set(CMAKE_CONFIGURATION_TYPES ${ACCEPTED_CONFIGURATION_TYPES} CACHE INTERNAL "")
    else()
        if (NOT CMAKE_BUILD_TYPE)
            message(STATUS "Setting build type to 'Debug' as none was specified.")
            set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
        endif()
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${ACCEPTED_CONFIGURATION_TYPES})
    endif()
endif()

# Cleanup
if (generatorIsMultiConfig)
    unset(CMAKE_BUILD_TYPE CACHE)
else()
    unset(CMAKE_CONFIGURATION_TYPES CACHE)
endif()


list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CMakeDependentOption)
include(cmake/Custom.cmake)
include(cmake/GetGitRevisionDescription.cmake)
include(cmake/GitDate.cmake)
include(cmake/WriteBinaryVersionInfo.cmake)


# PLATFORM AND ARCHITECTURE

# Architecture (32/64 bit)
set(X64 OFF)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64 ON)
endif()


set(DEFAULT_COMPILE_DEFS
    # Debug build, assert enabled: Debug, RelNoOptimization
    $<$<CONFIG:Release>:NDEBUG>          # optimized builds
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMPILER_IS_CLANG YES)
else()
    set(COMPILER_IS_CLANG NO)
endif()

# Setup platform specifics (compile flags, etc., ...)
if(MSVC)
    if(COMPILER_IS_CLANG)
        include(cmake/PlatformWindowsMSVCClang.cmake)
    else()
        include(cmake/PlatformWindowsMSVC.cmake)
    endif()
elseif(UNIX)
    if (COMPILER_IS_CLANG)
        include(cmake/PlatformLinuxClang.cmake)
    else()
        include(cmake/PlatformLinuxGCC.cmake)
    endif()
else()
    message(WARNING "Unsupported platform/compiler combination")
endif()



#
# Deployment/installation setup
#

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
    set(INSTALL_THIRDPARTY_LICENSES "licenses")

elseif(UNIX)
    set(OPTION_LOCAL_INSTALL ON CACHE INTERNAL "Local installation enforced, packaging is not supported yet on linux systems.")

#     set(INSTALL_ROOT "share/${META_PROJECT_NAME}")        # /usr/[local]/share/<project>
#     set(INSTALL_DATA "share/${META_PROJECT_NAME}")        # /usr/[local]/share/<project>
    set(INSTALL_ROOT ".")
    set(INSTALL_DATA "data")
    set(INSTALL_BIN "bin")                      # /usr/[local]/bin
    set(INSTALL_SHARED "lib")                   # /usr/[local]/lib
    set(INSTALL_LIB "lib")                      # /usr/[local]/lib
    set(INSTALL_INCLUDE "include")              # /usr/[local]/include
    set(INSTALL_DOC "share/doc/${META_PROJECT_NAME}")     # /usr/[local]/share/doc/<project>
    set(INSTALL_SHORTCUTS "share/applications") # /usr/[local]/share/applications
    set(INSTALL_ICONS "share/pixmaps")          # /usr/[local]/share/pixmaps
    set(INSTALL_INIT "/etc/init")               # /etc/init (upstart init scripts)
    set(INSTALL_PLUGINS_BIN "${INSTALL_BIN}/plugins")
    set(INSTALL_PLUGINS_SHARED "${INSTALL_SHARED}/plugins")
    set(INSTALL_PLUGINS_LIB "${INSTALL_LIB}/plugins")
    set(INSTALL_THIRDPARTY_LICENSES "licenses")

    install(PROGRAMS packages/${META_PROJECT_NAME}.sh
        DESTINATION ${INSTALL_ROOT})

    # Prefer using libraries in ./lib/ folder when installed.
    # (see https://cmake.org/Wiki/CMake_RPATH_handling)
    set(CMAKE_SKIP_BUILD_RPATH OFF) # use, i.e. don't skip the full RPATH for the build tree
    set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF) # when building, don't use the install RPATH already (but later on when installing)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${INSTALL_LIB}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
else()
    message(FATAL_ERROR "Platform not supported.")
endif()


# Install the project meta files
#install(FILES geohazardvis-config.cmake DESTINATION ${INSTALL_ROOT})
#install(FILES AUTHORS DESTINATION               ${INSTALL_ROOT})
install(FILES LICENSE DESTINATION               ${INSTALL_ROOT})


include(cmake/FindThirdParties.cmake)


# Get git revision
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING "${GIT_SHA1}" 0 12 GIT_REV)
get_git_commit_date(${GIT_SHA1} GIT_DATE)
set(GIT_REVISION_FILE ${PROJECT_BINARY_DIR}/revision)
set(VERSION_INFO_FILE ${PROJECT_BINARY_DIR}/vcs_commit_info.h)
# Write git information for this project
configure_file(cmake/vcs_commit_info.h.in ${VERSION_INFO_FILE})


set(CONFIG_FILE ${PROJECT_BINARY_DIR}/core/config.h)
configure_file(cmake/config.h.in ${CONFIG_FILE})

generate_cppcheck_suppressions()


set(IDE_PLUGINS_FOLDER "Plugins")
set(IDE_TESTS_FOLDER "Tests")
set(IDE_CPPCHECK_FOLDER "${IDE_TESTS_FOLDER}/Cppcheck")
set(IDE_DEPLOYMENT_FOLDER "Deployment, Documentation")
set(IDE_DOC_FOLDER ${IDE_DEPLOYMENT_FOLDER})
set(IDE_TOOLS_FOLDER "Tools")

set(IDE_FOLDER "")
add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("data")

# Plugins that require additional third party libraries may add their required runtime paths
# using the addPluginRuntimPathEntries function.
# Executable targets depend on that, to find required DLLs at runtime
include(cmake/ProjectConfigSetup.cmake)
set(IDE_FOLDER ${IDE_PLUGINS_FOLDER})
add_subdirectory("plugins")

set(IDE_FOLDER "")
add_subdirectory("app")
set(IDE_FOLDER ${IDE_TESTS_FOLDER})
add_subdirectory("tests")

set(IDE_FOLDER ${IDE_DOC_FOLDER})
add_subdirectory("docs")
set(IDE_FOLDER ${IDE_DEPLOYMENT_FOLDER})
add_subdirectory("packages")

if (OPTION_BUILD_TOOLS)
    set(IDE_FOLDER ${IDE_TOOLS_FOLDER})
    add_subdirectory("tools")
endif()

set(THIRD_PARTY_INFO_HEADER "${CMAKE_BINARY_DIR}/third_party_info.h")
write_third_party_config_file(${THIRD_PARTY_INFO_HEADER})

create_cppcheck_ALL_target()
