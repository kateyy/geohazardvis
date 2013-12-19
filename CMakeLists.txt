cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set (META_PROJECT_NAME "GeohazardVis")
set (META_VERSION_MAJOR "0")
set (META_VERSION_MINOR "0")
set (META_VERSION_PATCH "0")
set (META_VERSION "${META_VERSION_MAJOR}.${META_VERSION_MINOR}.${META_VERSION_PATCH}")
set (META_AUTHOR_ORGANIZATION "")
set (META_AUTHOR_DOMAIN "https://github.com/kateyy/geohazardvis")

project (${META_PROJECT_NAME} C CXX)

option(OPTION_LIMIT_CONFIGS "Generate limited configs (Release; Debug)" ON)
option(OPTION_LOCAL_INSTALL "Install to local directory instead of system" OFF)

if(OPTION_LIMIT_CONFIGS)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited Configs" FORCE)
endif()



set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_MODULE_PATH 
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    
    
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

    

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources     # for the cone
  vtkInteractionWidgets # vtkBoxWidget
  vtkRenderingCore      # vtkPlyDataMapper
  vtkRenderingOpenGL    # not loading this simply causes seg faults
  vtkViewsQt
  vtkFiltersCore
  vtkInfovisCore
  vtkInteractionStyle
  vtkRenderingFreeTypeOpenGL
)
include(${VTK_USE_FILE})

set(CMAKE_AUTOMOC ON)
set(AUTOMOC_MOC_OPTIONS PROPERTIES FOLDER CMakeAutomocTargets)

set_property(GLOBAL PROPERTY AUTOMOC_FOLDER CMakeAutomocTargets)

if("${VTK_QT_VERSION}" STREQUAL "")
    message(FATAL_ERROR "VTK was not built with Qt")
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory("core")
add_subdirectory("gui")
add_subdirectory("app")
