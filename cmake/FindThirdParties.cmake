
# FindThirdParties.cmake
#
# This module finds required third party packages.
# Include it in the top CMakeLists.txt file of the project to include third party variables in the
# project scope!
#
# Module Dependencies:
#   * ExternalProjectMapBuild.cmake
#   * GetGitRevisionDescription.cmake
#   * GitDate.cmake
#
#
# Handled external dependencies:
#   * VTK    -> VTK_DEFINITIONS, VTK_LIBRARIES, VTK_INCLUDE_DIRS, (and some detailed options such
#               as VTK_RENDERING_BACKEND)
#   * Qt5    -> Qt5::* imported targets, PROJECT_QT_COMPONENTS
#   * proj.4 -> PROJ4_INCLUDE_FILE links the a generated include file that include proj.4.
#               By default, the proj.4 version packaged with VTK is used.
#               If OPTION_USE_PROJ4_FROM_VTK is disabled, PROJ4_FOUND, PROJ4_INCLUDE_DIR and
#               PROJ4_LIBRARIES will be defined.
#
#
# Packages either handled as ExternalProject (default) or by find_package
#   OPTION_USE_SYSTEM_*     - ON: find_package(*), OFF: build as external project
#   OPTION_*_GIT_REPOSITORY - if OPTION_USE_SYSTEM_* is OFF: adjust the source git folder or URL
#                             for the package
#
#   * libzeug   -> LIBZEUG_INCLUDES, LIBZEUG_LIBRARIES
#
#   Only relevant if tests are enabled (OPTION_BUILD_TESTS):
#   * gtest     -> GTEST_FOUND GTEST_INCLUDE_DIR GTEST_LIBRARIES
#
#
# Third party packaging: OPTION_INSTALL_3RDPARTY_BINARIES
#   Enable this option (default) to include third party runtime libraries packages and
#   installations. This requires CMake scripts that are partly quite specific for the each
#   package/OS, so please double check if the package actually runs on the relevant platform.
#


set(THIRD_PARTY_SOURCE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty")
set(THIRD_PARTY_BUILD_DIR "${PROJECT_BINARY_DIR}/ThirdParty")
include(ExternalProject)


# helper functions


# Read git revision and date.
# This only works if BUILD_DIR refers to the build directory, not an installed version of package
function(get_source_dir_from_build PACKAGE_NAME BUILD_DIR _SOURCE_DIR)
    set(cacheFile "${BUILD_DIR}/CMakeCache.txt")
    if (NOT EXISTS ${cacheFile})
        set(${_SOURCE_DIR} "" PARENT_SCOPE)
        return()
    endif()
    set(ts_var cachedSourceDirTimestamp_${PACKAGE_NAME})
    set(cachedResultVar cachedSourceDir_${PACKAGE_NAME})
    file(TIMESTAMP ${cacheFile} newTimestamp)
    if (${ts_var} AND ${ts_var} STREQUAL newTimestamp)  # cached
        set(${_SOURCE_DIR} ${${cachedResultVar}} PARENT_SCOPE)
        return()
    endif()
    # need to read again
    set(${ts_var} ${newTimestamp} CACHE INTERNAL "" FORCE)
    file(STRINGS ${cacheFile} cache_entries)
    foreach(entry ${cache_entries})
        string(FIND "${entry}" "CMAKE_HOME_DIRECTORY:INTERNAL=" in_str_idx)
        if (in_str_idx EQUAL -1)
            continue()
        endif()
        string(SUBSTRING "${entry}" 30 -1 entry)
        set(${_SOURCE_DIR} ${entry} PARENT_SCOPE)
        set(${cachedResultVar} ${entry} CACHE INTERNAL "" FORCE)
        break()
    endforeach()
endfunction()

set(THIRD_PARTY_PACKAGES "" CACHE INTERNAL "")
function(deploy_license_file PACKAGE_NAME FILE_PATH)
    install(FILES ${FILE_PATH} DESTINATION ${INSTALL_THIRDPARTY_LICENSES}
        RENAME "${PACKAGE_NAME}.txt")
    list(APPEND THIRD_PARTY_PACKAGES "${PACKAGE_NAME}")
    set(THIRD_PARTY_PACKAGES ${THIRD_PARTY_PACKAGES} CACHE INTERNAL "")
endfunction()

function(set_third_party_info name sha1 rev date)
    set(THIRD_PARTY_${name}_hasInfo TRUE CACHE INTERNAL "")
    set(THIRD_PARTY_${name}_sha1 ${sha1} CACHE INTERNAL "")
    set(THIRD_PARTY_${name}_rev ${rev} CACHE INTERNAL "")
    set(THIRD_PARTY_${name}_date ${date} CACHE INTERNAL "")
endfunction()

function(write_third_party_config_file outputFileName)
    set(THIRD_PARTY_NAME_STRINGS)
    foreach(name ${THIRD_PARTY_PACKAGES})
        if (THIRD_PARTY_${name}_hasInfo)
            string(APPEND THIRD_PARTY_STRUCT_INITS
                "    { \"${name}\", true, \"${THIRD_PARTY_${name}_sha1}\", \
\"${THIRD_PARTY_${name}_rev}\", \"${THIRD_PARTY_${name}_date}\" },\n")
        else()
            string(APPEND THIRD_PARTY_STRUCT_INITS
                "    { \"${name}\", false, {}, {}, {} },\n")
        endif()
    endforeach()
    list(LENGTH THIRD_PARTY_PACKAGES NUM_THIRD_PARTIES)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/third_party_info.h.in ${outputFileName})
endfunction()


#========= VTK =========

set(VTK_COMPONENTS
    vtkChartsCore
    vtkFiltersPoints
    vtkInteractionWidgets
    vtkIOXML
    vtkRenderingAnnotation  # vtkCubeAxesActor, vtkScalarBarActor
    vtkViewsContext2D
)

if (OPTION_USE_QVTKOPENGLWIDGET)
    list(APPEND VTK_COMPONENTS vtkGUISupportQt)       # QVTKOpenGLWidget
else()
    list(APPEND VTK_COMPONENTS vtkGUISupportQtOpenGL) # QVTKWidget2
endif()


find_package(VTK REQUIRED COMPONENTS ${VTK_COMPONENTS})

set(VTK_VERSION "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
set(VTK_REQUIRED_VERSION 7.1)

if (VTK_VERSION VERSION_LESS VTK_REQUIRED_VERSION)
    message(FATAL_ERROR "VTK version ${VTK_VERSION} was found but version ${VTK_REQUIRED_VERSION} or newer is required.")
else()
    message(STATUS "Searching VTK version ${VTK_REQUIRED_VERSION} (found ${VTK_VERSION})")
endif()

# replaced by explicit calls on the relevant targets (http://www.kitware.com/source/home/post/116)
#include(${VTK_USE_FILE})

if (NOT "${VTK_QT_VERSION}" STREQUAL "5")
    message(FATAL_ERROR "VTK was built with Qt version ${VTK_QT_VERSION}, but Qt5 is required.")
endif()

if ("${VTK_RENDERING_BACKEND}" STREQUAL "")
    set(VTK_RENDERING_BACKEND "OpenGL")
endif()
if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL" OR "${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
    if (${VTK_RENDERING_BACKEND} STREQUAL "OpenGL")
        set(VTK_RENDERING_BACKEND_VERSION 1)
        set(VTK_RENDERING_BACKEND_INFO "OpenGL (legacy)")
    else()
        set(VTK_RENDERING_BACKEND_VERSION 2)
        set(VTK_RENDERING_BACKEND_INFO "OpenGL2")
    endif()
    message(STATUS "    Rendering Backend: ${VTK_RENDERING_BACKEND_INFO}")
else()
    message(FATAL_ERROR "Unsupported VTK Rendering Backend: ${VTK_RENDERING_BACKEND}")
endif()


# OpenGL backend dependent modules
list(APPEND VTK_COMPONENTS
    vtkIOExport${VTK_RENDERING_BACKEND}
    vtkRenderingContext${VTK_RENDERING_BACKEND}
)
if (VTK_RENDERING_BACKEND_VERSION EQUAL 1)
    list(APPEND VTK_COMPONENTS vtkRenderingGL2PS)
else()
    list(APPEND VTK_COMPONENTS vtkRenderingGL2PSOpenGL2)
endif()

# configuration dependent VTK components

if (OPTION_ENABLE_LIC2D)
    if (VTK_RENDERING_BACKEND_VERSION EQUAL 2)
        list(APPEND VTK_COMPONENTS vtkRenderingLICOpenGL2)
        list(APPEND VTK_COMPONENTS vtkRenderingContextOpenGL2)
    else()
        list(APPEND VTK_COMPONENTS vtkRenderingLIC)
    endif()
endif()


if (OPTION_ENABLE_TEXTURING)
    list(APPEND VTK_COMPONENTS vtkFiltersTexture)
endif()


# Use better font rending/unicode support if available
if (TARGET vtkRenderingFreeTypeFontConfig)
    list(APPEND VTK_COMPONENTS vtkRenderingFreeTypeFontConfig)
elseif(UNIX)
    message("VTK is built without vtkRenderingFreeTypeFontConfig. "
        "This limits font rendering to ASCII characters.")
endif()


#== proj.4 == (prefer VTK's bundled proj.4)
option(OPTION_USE_PROJ4_FROM_VTK "Link to proj.4 bundled with VTK. \
This only works if VTK_DIR links to a VTK build directory, because VTK does not install its \
proj.4 library." ON)
mark_as_advanced(OPTION_USE_PROJ4_FROM_VTK)
if (OPTION_USE_PROJ4_FROM_VTK)
    list(APPEND VTK_COMPONENTS vtklibproj4)
    set(proj4_package_include_file "vtk_libproj4.h")
else()
    find_package(PROJ4 REQUIRED)
    set(proj4_package_include_file "proj_api.h")
endif()
# NOTE: PROJ4_INCLUDE_FILE should be added to the core library.
set(PROJ4_INCLUDE_FILE "${PROJECT_BINARY_DIR}/core/ThirdParty/proj4_include.h")
configure_file(${PROJECT_SOURCE_DIR}/cmake/proj4_include.h.in ${PROJ4_INCLUDE_FILE})


# find additional VTK components
message(STATUS "    Requested components: ${VTK_COMPONENTS}")
find_package(VTK REQUIRED COMPONENTS ${VTK_COMPONENTS})

if (OPTION_USE_PROJ4_FROM_VTK)
    set(proj4_includes_exist 0)
    foreach(inc ${vtklibproj4_INCLUDE_DIRS})
        if (EXISTS "${inc}/vtklibproj4/src/proj_api.h")
            set(proj4_includes_exist 1)
            break()
        endif()
    endforeach()
    if (NOT proj4_includes_exist)
        message(FATAL_ERROR "Could not find PROJ.4 includes in VTK include directories. \
See OPTION_USE_PROJ4_FROM_VTK")
    endif()
endif()

# When using VTK kits, the provided library names are just interfaces to underlaying targets
# Restore actual library target to get properties from them.
if (VTK_ENABLE_KITS)
    set(actualVTKLibraries)
    foreach(libName ${VTK_LIBRARIES})
        set(kitName ${${libName}_KIT})
        if (kitName)
            list(APPEND actualVTKLibraries ${kitName})
        else()
            list(APPEND actualVTKLibraries ${libName})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES actualVTKLibraries)
else()
    set(actualVTKLibraries ${VTK_LIBRARIES})
endif()

# RelNoOptimization erroneously links to Debug by default.
# Fix this by setting RelNoOptimization properties (redirecting to RelWithDebInfo)
foreach(libName ${actualVTKLibraries})
    if (NOT TARGET ${libName})
        continue()
    endif()
    set(requiredOptions
        INTERFACE_LINK_LIBRARIES
        IMPORTED_IMPLIB_RELWITHDEBINFO
        IMPORTED_LOCATION_RELWITHDEBINFO
        IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO
        IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO
    )
    foreach(key ${requiredOptions})
        string(REPLACE RELWITHDEBINFO RELNOOPTIMIZATION newKey ${key})
        get_target_property(value ${libName} ${key})
        if (value)
            set_target_properties(${libName}
                PROPERTIES ${newKey} "${value}")
        endif()
    endforeach()
endforeach()


# Read VTK git revision and date.
get_source_dir_from_build("VTK" ${VTK_DIR} VTK_SOURCE_DIR)
if (VTK_SOURCE_DIR)
    get_git_head_revision(VTK_GIT_REFSPEC VTK_GIT_SHA1 ${VTK_SOURCE_DIR})
    if (NOT VTK_GIT_SHA1 EQUAL 000000000000)
        string(SUBSTRING "${VTK_GIT_SHA1}" 0 12 VTK_GIT_REV)
        get_git_commit_date(${VTK_GIT_SHA1} VTK_GIT_DATE ${VTK_SOURCE_DIR})
    endif()
    deploy_license_file("VTK" "${VTK_SOURCE_DIR}/Copyright.txt")
    set_third_party_info("VTK" ${VTK_GIT_SHA1} ${VTK_GIT_REV} ${VTK_GIT_DATE})
endif()
# Some components of ParaView are used
deploy_license_file("ParaView" "${PROJECT_SOURCE_DIR}/core/ThirdParty/ParaView/License_v1.2_ParaViewOnly.txt")


#========= Qt5 =========

set(PROJECT_QT_COMPONENTS Core Gui Widgets Concurrent)
if (NOT OPTION_USE_QVTKOPENGLWIDGET)
    list(APPEND PROJECT_QT_COMPONENTS OpenGL)
endif()
if (UNIX)
    # required for platform plugin deployment
    list(APPEND PROJECT_QT_COMPONENTS DBus Svg X11Extras)
endif()
find_package(Qt5 5.5 REQUIRED COMPONENTS ${PROJECT_QT_COMPONENTS})

find_program(Qt5QMake_PATH qmake
    DOC "Path to the qmake executable of the currently used Qt5 installation.")

set(CMAKE_AUTOMOC ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated")



#========= libzeug =========

if (CMAKE_VERSION VERSION_LESS 3.6)
    set(_git_shallow_flag)
else()
    set(_git_shallow_flag GIT_SHALLOW 1)
endif()

option(OPTION_USE_SYSTEM_LIBZEUG "Search for installed libzeug libraries instead of compiling them" OFF)
CMAKE_DEPENDENT_CACHEVARIABLE(OPTION_LIBZEUG_GIT_REPOSITORY "https://github.com/kateyy/libzeug.git" STRING
    "URL/path of the libzeug git repository" "NOT OPTION_USE_SYSTEM_LIBZEUG" OFF)

if (OPTION_USE_SYSTEM_LIBZEUG)
    find_package(libzeug REQUIRED)
else()
    set(LIBZEUG_FOUND 1)
    set(LIBZEUG_PREFIX "${THIRD_PARTY_BUILD_DIR}/libzeug")
    set(libzeug_DIR "${LIBZEUG_PREFIX}/install")

    set(_cmakeCacheArgs
        -DOPTION_BUILD_TESTS:bool=OFF
        -DOPTION_USE_OPENMP:bool=OFF
        -DQt5Core_DIR:path=${Qt5Core_DIR}
        -DQt5Gui_DIR:path=${Qt5Gui_DIR}
        -DQt5OpenGL_DIR:path=${Qt5OpenGL_DIR}
        -DQt5Widgets_DIR:path=${Qt5Widgets_DIR}
        -DCMAKE_INSTALL_PREFIX:path=${libzeug_DIR}
        -DCMAKE_BUILD_TYPE:string=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER:filepath=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:filepath=${CMAKE_C_COMPILER}
    )
    if (MSVC)
        list(APPEND _cmakeCacheArgs
            -DOPTION_RELEASE_LTCG:bool=${OPTION_RELEASE_LTCG}
        )
    endif()

    ExternalProject_Add(libzeug
        PREFIX ${LIBZEUG_PREFIX}
        SOURCE_DIR ${THIRD_PARTY_SOURCE_DIR}/libzeug
        GIT_REPOSITORY ${OPTION_LIBZEUG_GIT_REPOSITORY}
        GIT_TAG "geohazardvis_2016_06"
        ${_git_shallow_flag}
        BUILD_COMMAND ${CMAKE_COMMAND}
            -DBUILD_CONFIG:STRING=$<CONFIG>
            -P ${PROJECT_SOURCE_DIR}/cmake/ExternalProjectMapBuild.cmake
        INSTALL_COMMAND ""
        CMAKE_CACHE_ARGS ${_cmakeCacheArgs}
        USES_TERMINAL_DOWNLOAD 1
        USES_TERMINAL_UPDATE 1
        USES_TERMINAL_CONFIGURE 1
        USES_TERMINAL_BUILD 1
    )

    set(LIBZEUG_INCLUDES ${libzeug_DIR}/include)

    if (WIN32)
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

if (WIN32 OR OPTION_USE_SYSTEM_LIBZEUG)
    deploy_license_file("libzeug" "${libzeug_DIR}/LICENSE")
else()
    deploy_license_file("libzeug" "${libzeug_DIR}/share/libzeug/LICENSE")
endif()


#========= Third Party Deployment =========

if (OPTION_INSTALL_3RDPARTY_BINARIES)
    include(cmake/DeploySystemLibraries.cmake)

    if (VTK_BUILD_SHARED_LIBS)
        set(_vtkBuiltInLibraries)
        set(_vtkFromSystemLibraries)
        foreach(_libName ${actualVTKLibraries})
            # For libraries built within VTK just their names are listed. Thus, absolute library
            # paths refer to system libraries that VTK libraries are linked to.
            # See VTK_USE_SYSTEM_ZLIB, etc.
            if (IS_ABSOLUTE ${_libName})
                # VTK CMake configuration might be use symlinks, but we need to install the actual files.
                if (IS_SYMLINK ${_libName})
                    get_filename_component(_libName "${_libName}" REALPATH)
                endif()
                list(APPEND _vtkFromSystemLibraries ${_libName})
            else()
                list(APPEND _vtkBuiltInLibraries ${_libName})
            endif()
        endforeach()
        if (_vtkFromSystemLibraries)
            message(STATUS "Assuming that the following system libraries are linked by VTK and need to be deployed")
            message(STATUS "(Check VTK_USE_SYSTEM_* options in your VTK CMake setup):")
            foreach(_lib ${_vtkFromSystemLibraries})
                get_filename_component(_libShort "${_lib}" NAME)
                # HACK: VTK links to libraries names such as libz.so.1, so we must truncate minor
                # version numbers, such as in libz.so.1.2.11
                # TODO: would be better to detect the real link names using ldd or whatever.
                string(REGEX REPLACE "\\.[0-9]+\\.[0-9]+$" "" _libShort ${_libShort})
                message(STATUS "    ${_lib} -> ${_libShort}")
                install(FILES ${_lib} DESTINATION ${INSTALL_SHARED} RENAME ${_libShort})
            endforeach()
        endif()
        set(_vtkDeployFiles)
        if (WIN32)
            # Windows: VTK build tree contains separated folders for each configuration
            foreach(libName ${_vtkBuiltInLibraries})
                list(APPEND _vtkDeployFiles "${VTK_DIR}/bin/$<$<STREQUAL:$<CONFIG>,RelNoOptimization>:RelWithDebInfo>$<$<NOT:$<STREQUAL:$<CONFIG>,RelNoOptimization>>:$<CONFIG>>/${_libName}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
            endforeach()
        elseif(UNIX)
            # Linux: configuration type of installed VTK must be the same as the deployment configuration for this project.
            # We have to rely on this correct setup here.
            foreach(_libName ${_vtkBuiltInLibraries})
                get_filename_component(_realPath "lib${_libName}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.so" REALPATH
                    BASE_DIR ${VTK_DIR}/lib)
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


#========= GTest =========

CMAKE_DEPENDENT_OPTION(OPTION_USE_SYSTEM_GTEST "Search for installed Google Test Libraries instead of compiling them" OFF
    "OPTION_BUILD_TESTS" OFF)
CMAKE_DEPENDENT_CACHEVARIABLE(OPTION_GTEST_GIT_REPOSITORY "https://github.com/google/googletest" STRING
    "URL/path of the gtest git repository" "NOT OPTION_USE_SYSTEM_GTEST;OPTION_BUILD_TESTS" OFF)

if (OPTION_BUILD_TESTS)

    if (OPTION_USE_SYSTEM_GTEST)
        find_package(GTEST)
        find_package(GMOCK)
    else()
        set(GTEST_SOURCE_DIR ${THIRD_PARTY_SOURCE_DIR}/gtest)
        set(GTEST_BUILD_DIR ${THIRD_PARTY_BUILD_DIR}/gtest)
        set(GTEST_INSTALL_DIR ${GTEST_BUILD_DIR}/install-$<CONFIG>)

        if (WIN32)
            set(_gtestLib "${GTEST_INSTALL_DIR}/lib/gtest.lib")
            set(_gmockLib "${GTEST_INSTALL_DIR}/lib/gmock.lib")
        else()
            set(_gtestLib "${GTEST_INSTALL_DIR}/lib/libgtest.a")
            set(_gmockLib "${GTEST_INSTALL_DIR}/lib/libgmock.a")
        endif()

        set(GTEST_FOUND 1)
        set(GTEST_INCLUDE_DIR ${GTEST_INSTALL_DIR}/include)
        set(GTEST_LIBRARIES ${_gtestLib})

        set(GMOCK_FOUND 1)
        set(GMOCK_INCLUDE_DIR ${GTEST_INCLUDE_DIR})
        set(GMOCK_LIBRARIES ${_gmockLib})

        ExternalProject_Add(gtest
            PREFIX ${GTEST_BUILD_DIR}
            SOURCE_DIR ${GTEST_SOURCE_DIR}
            GIT_REPOSITORY ${OPTION_GTEST_GIT_REPOSITORY}
            GIT_TAG "release-1.8.0"
            ${_git_shallow_flag}
            BUILD_COMMAND ${CMAKE_COMMAND}
                -DBUILD_CONFIG:STRING=$<CONFIG>
                -P ${PROJECT_SOURCE_DIR}/cmake/ExternalProjectMapBuild.cmake
            INSTALL_COMMAND ""
            CMAKE_CACHE_ARGS
                -Dgtest_force_shared_crt:bool=ON
                -DCMAKE_CXX_COMPILER:filepath=${CMAKE_CXX_COMPILER}
                -DCMAKE_C_COMPILER:filepath=${CMAKE_C_COMPILER}
                -DBUILD_GMOCK:bool=ON
                -DBUILD_GTEST:bool=OFF
                # without setting this explicitly the test/mock subproject don't install correctly
                -DCMAKE_INSTALL_PREFIX:path=${GTEST_INSTALL_DIR}
            USES_TERMINAL_DOWNLOAD 1
            USES_TERMINAL_UPDATE 1
            USES_TERMINAL_CONFIGURE 1
            USES_TERMINAL_BUILD 1
        )
    endif()

    if (NOT GTEST_FOUND)
        message(WARNING "Tests skipped: gtest not found")
    endif()
endif()


# Further third party licenses to deploy
deploy_license_file("Alphanum" "${PROJECT_SOURCE_DIR}/core/ThirdParty/alphanum_license.txt")
