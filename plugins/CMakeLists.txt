
option(OPTION_BUILD_PLUGINS "Include plugins located in the plugins/ source folder in the build process" ON)

if(NOT OPTION_BUILD_PLUGINS)
    return()
endif()


file(GLOB folderContents RELATIVE "${CMAKE_CURRENT_LIST_DIR}/" "*")

set(plugins "")
foreach(subdir ${folderContents})
    set(dir "${CMAKE_CURRENT_LIST_DIR}/${subdir}")
    if(IS_DIRECTORY ${dir} AND EXISTS "${dir}/CMakeLists.txt")
        list(APPEND plugins ${subdir})
    endif()
endforeach()


set(GEOHAZARDVIS_INCLUDE_DIR ${CMAKE_SOURCE_DIR})

foreach(pluginname ${plugins})
    
    CMAKE_DEPENDENT_OPTION(OPTION_BUILD_PLUGIN_${pluginname} "Build plugin: ${pluginname}" ON
        "OPTION_BUILD_PLUGINS" OFF)
    
    if(OPTION_BUILD_PLUGIN_${pluginname})
        add_subdirectory(${pluginname})
    endif()
endforeach()
