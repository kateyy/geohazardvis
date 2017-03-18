
message(STATUS "Qt Designer Plugins")

option(OPTION_BUILD_QT_DESIGNER_PLUGINS "Build Qt Designer plugins for custom widgets." ON)
mark_as_advanced(OPTION_BUILD_QT_DESIGNER_PLUGINS)

if (!OPTION_BUILD_QT_DESIGNER_PLUGINS)
    return()
endif()


set(qt_component UiPlugin)
if (Qt5_VERSION VERSION_LESS "5.5.0")
  set(qt_component Designer)
endif()

find_package(Qt5 QUIET COMPONENTS ${qt_component})

if (NOT Qt5UiPlugin_FOUND)
    message("Not building Qt Designer plugins, as the Qt5UiPlugin component was not found. "
        "Disable OPTION_BUILD_QT_DESIGNER_PLUGINS to suppress this warning.")
    return()
endif()


set(deployTargetName "DeployQt5DesignerPlugins")
set(OPTION_Qt5Designer_plugins_deploy_dir "" CACHE PATH
    "Deployment path for Qt5 Designer plugins (build target ${deployTargetName})")

set(pluginsIDEFolder "Qt Designer Plugins")

add_custom_target(${deployTargetName})
set_target_properties(${deployTargetName}
    PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD 1
    FOLDER ${pluginsIDEFolder}
)
if(OPTION_Qt5Designer_plugins_deploy_dir)
    add_custom_command(TARGET ${deployTargetName} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
            "Deploying Qt5 Designer plugins to: ${OPTION_Qt5Designer_plugins_deploy_dir}")
else()
    add_custom_command(TARGET ${deployTargetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
            "Set OPTION_Qt5Designer_plugins_deploy_dir in CMake to deploy Qt5 Designer plugins!")
endif()

function(add_designer_plugin CLASS_NAME)
    set(pluginName "${CLASS_NAME}Plugin")
    add_library(${pluginName} SHARED
        ${pluginName}.h
        ${pluginName}.cpp
        ../${CLASS_NAME}.h
        ../${CLASS_NAME}.cpp
    )
    target_compile_definitions(${pluginName} PRIVATE COMPILE_QT_DESIGNER_PLUGIN)
    target_link_libraries(${pluginName}
        PUBLIC Qt5::${qt_component}
    )
    configure_cxx_target(${pluginName} IDE_FOLDER ${pluginsIDEFolder})

    if (OPTION_Qt5Designer_plugins_deploy_dir)
        add_custom_command(TARGET ${deployTargetName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo " - ${pluginName}"
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:${pluginName}> "${OPTION_Qt5Designer_plugins_deploy_dir}")
    endif()
    add_dependencies(${deployTargetName} ${pluginName})

endfunction()

add_designer_plugin(CollapsibleGroupBox)
add_designer_plugin(DoubleSpinBox)
