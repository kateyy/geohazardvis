set(target plugin_template)

message(STATUS "Plugin ${target}")


set(sources
    plugin.cpp
    PluginTemplate.h
    PluginTemplate.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")


add_library(${target} SHARED ${sources})

target_include_directories(${target}
    PUBLIC ${GEOHAZARDVIS_INCLUDE_DIR}
)

target_link_libraries(${target}
    gui
)

configure_cxx_plugin(${target})


# install(TARGETS ${target}
    # RUNTIME DESTINATION ${INSTALL_PLUGINS_BIN}
    # LIBRARY DESTINATION ${INSTALL_PLUGINS_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_PLUGINS_LIB}
# )
