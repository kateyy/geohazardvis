
# FindThirdParties.cmake
#
# This module finds required third party packages.
# Include it in the top CMakeLists.txt file of the project to include third party variables in the project scope!
#
#
# Handled external dependencies:
#   * OpenGL    -> OPENGL_INCLUDE_DIR, OPENGL_LIBRARIES
#   * VTK       -> VTK_DEFINITIONS, VTK_LIBRARIES, VTK_INCLUDE_DIRS, (and some detailed options such as VTK_RENDERING_BACKEND)
#   * Qt5       -> Qt5::* imported targets, PROJECT_QT_COMPONENTS
#
#
# Packages either handled as ExternalProject (default) or by find_package
#   OPTION_USE_SYSTEM_*     - ON: find_package(*), OFF: build as external project
#   OPTION_*_GIT_REPOSITORY - if OPTION_USE_SYSTEM_* is OFF: adjust the source git folder or URL for the package
#
#   * libzeug   -> LIBZEUG_INCLUDES, LIBZEUG_LIBRARIES
#
#   Only relevant if tests are enabled (OPTION_BUILD_TESTS):
#   * gtest     -> GTEST_FOUND GTEST_INCLUDE_DIR GTEST_LIBRARIES
#
#
# Third party packaging: OPTION_INSTALL_3RDPARTY_BINARIES
#   Enable this option (default) to include third party runtime libraries packages and installations.
#   This requires CMake scripts that are partly quite specific for the each package/OS, so please
#   double check if the package actually runs on the relevant platform.
#


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

# replaced by explicit calls on the relevant targets (http://www.kitware.com/source/home/post/116)
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


set(PROJECT_QT_COMPONENTS Core Gui Widgets OpenGL)
if (UNIX)
    # required for platform plugin deployment
    list(APPEND PROJECT_QT_COMPONENTS DBus Svg X11Extras)
endif()
find_package(Qt5 5.3 COMPONENTS ${PROJECT_QT_COMPONENTS})
if(NOT ${Qt5Core_VERSION} VERSION_LESS 5.5)
    find_package(Qt5 COMPONENTS Concurrent)
    list(APPEND PROJECT_QT_COMPONENTS Concurrent)
endif()

find_program(Qt5QMake_PATH qmake
    DOC "Path to the qmake executable of the currently used Qt5 installation.")

set(CMAKE_AUTOMOC ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated")


if (CMAKE_VERSION VERSION_LESS 3.6)
    set(_git_shallow_flag)
else()
    set(_git_shallow_flag GIT_SHALLOW 1)
endif()

option(OPTION_USE_SYSTEM_LIBZEUG "Search for installed libzeug libraries instead of compiling them" OFF)
CMAKE_DEPENDENT_CACHEVARIABLE(OPTION_LIBZEUG_GIT_REPOSITORY "https://github.com/kateyy/libzeug.git" STRING
    "URL/path of the libzeug git repository" "NOT OPTION_USE_SYSTEM_LIBZEUG" OFF)

if(OPTION_USE_SYSTEM_LIBZEUG)
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
            -DBUILD_CONFIG:STRING=${CMAKE_CFG_INTDIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/libzeugExternalProjectBuild.cmake
        INSTALL_COMMAND ""
        CMAKE_CACHE_ARGS ${_cmakeCacheArgs}
    )

    set(LIBZEUG_INCLUDES ${libzeug_DIR}/include)

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
        if (VTK_ENABLE_KITS)
            set(_actualVTKLibs)
            foreach(_libName ${VTK_LIBRARIES})
                set(_kitName ${${_libName}_KIT})
                if (_kitName)
                    list(APPEND _actualVTKLibs ${_kitName})
                else()
                    list(APPEND _actualVTKLibs ${_libName})
                endif()
            endforeach()
            list(REMOVE_DUPLICATES _actualVTKLibs)
        else()
            set(_actualVTKLibs ${VTK_LIBRARIES})
        endif()

        set(_vtkDeployFiles)
        if (WIN32)
            # Windows: VTK build tree contains separated folders for each configuration
            foreach(_libName ${_actualVTKLibs})
                list(APPEND _vtkDeployFiles "${VTK_DIR}/bin/$<$<STREQUAL:$<CONFIG>,RelNoOptimization>:RelWithDebInfo>$<$<NOT:$<STREQUAL:$<CONFIG>,RelNoOptimization>>:$<CONFIG>>/${_libName}-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.dll")
            endforeach()
        elseif(UNIX)
            # Linux: configuration type of installed VTK must be the same as the deployment configuration for this project.
            # We have to rely on this correct setup here.
            foreach(_libName ${_actualVTKLibs})
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



CMAKE_DEPENDENT_OPTION(OPTION_USE_SYSTEM_GTEST "Search for installed Google Test Libraries instead of compiling them" OFF
    "OPTION_BUILD_TESTS" OFF)
CMAKE_DEPENDENT_CACHEVARIABLE(OPTION_GTEST_GIT_REPOSITORY "https://github.com/google/googletest" STRING
    "URL/path of the gtest git repository" "NOT OPTION_USE_SYSTEM_GTEST" OFF)

if(OPTION_BUILD_TESTS)

    if (OPTION_USE_SYSTEM_GTEST)
        find_package(GTEST)
        find_package(GMOCK)
    else()
        set(GTEST_SOURCE_DIR ${THIRD_PARTY_SOURCE_DIR}/gtest)
        set(GTEST_BUILD_DIR ${THIRD_PARTY_BUILD_DIR}/gtest)
        set(GTEST_INSTALL_DIR ${GTEST_BUILD_DIR}/install-$<CONFIG>)

        if(WIN32)
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
            GIT_TAG "0a439623f75c029912728d80cb7f1b8b48739ca4"
            ${_git_shallow_flag}
            CMAKE_CACHE_ARGS
                -Dgtest_force_shared_crt:bool=ON
                -DCMAKE_CXX_COMPILER:filepath=${CMAKE_CXX_COMPILER}
                -DCMAKE_C_COMPILER:filepath=${CMAKE_C_COMPILER}
                -DBUILD_GMOCK:bool=ON
                -DBUILD_GTEST:bool=OFF
                # without setting this explicitly the test/mock subproject don't install correctly
                -DCMAKE_INSTALL_PREFIX:path=${GTEST_INSTALL_DIR}
        )
    endif()

    if(NOT GTEST_FOUND)
        message(WARNING "Tests skipped: gtest not found")
    endif()
endif()
