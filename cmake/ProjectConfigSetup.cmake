

if(MSVC)
    option(OPTION_MSVC_AUTOMATIC_PROJECT_CONFIG
        "Automatically setup user specific MSVC project configuration (environment, working directory)"
        ON)
endif()

if(NOT OPTION_MSVC_AUTOMATIC_PROJECT_CONFIG)

    function(setupProjectUserConfig TARGET)
    endfunction()

    function(addPluginRuntimePathEntries PATH_ENTRIES)
    endfunction()

    return()

endif()

# this is reset on each CMake configuration invocation
# Thus, include this file only once before adding the plugin subfolders.
# All executables that depend on plugin/thirdparty runtime paths should be added after the plugins.
set(PLUGIN_RUNTIME_PATHS "" CACHE INTERNAL "" FORCE)

function(addPluginRuntimePathEntries PATH_ENTRIES)
    if(PLUGIN_RUNTIME_PATHS)
        set(PLUGIN_RUNTIME_PATHS "${PLUGIN_RUNTIME_PATHS};${PATH_ENTRIES}" CACHE INTERNAL "" FORCE)
    else()
        set(PLUGIN_RUNTIME_PATHS "${PATH_ENTRIES}" CACHE INTERNAL "" FORCE)
    endif()
endfunction()


if (NOT DEFINED Qt5QMake_PATH)
    message(FATAL_ERROR "Qt5QMake_PATH must be set before including ProjectConfigSetup.cmake")
endif()

function(setupProjectUserConfig TARGET)

    if(NOT OPTION_MSVC_AUTOMATIC_PROJECT_CONFIG)
        return()
    endif()

    if(NOT MSVC_VERSION)
        return()
    endif()

    unset(MSVC_TOOLS_VERSION)
    if(MSVC_VERSION EQUAL 1800)
        set(MSVC_TOOLS_VERSION 12.0)
    elseif(MSVC_VERSION EQUAL 1900)
        set(MSVC_TOOLS_VERSION 14.0)
    endif()

    if(NOT MSVC_TOOLS_VERSION)
        message(WARNING "Project file setup is not supported for your Visual Studio version. For debugging, you have to manually setup your project working directories and environments!")

        return()
    endif()

    set(MSVC_LOCAL_DEBUGGER_WORKING_DIRECTORY "..\\..")

    # prepend executable paths for 3rdparty libraries to configuration specific PATHs

    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG_TYPE} UPPER_CONFIG_TYPE)

        set(PROJECT_RUNTIME_PATHS)

        # VTK
        if(CONFIG_TYPE STREQUAL "RelNoOptimization")
            set(_vtkConfig "RelWithDebInfo")
        else()
            set(_vtkConfig ${CONFIG_TYPE})
        endif()
        list(APPEND PROJECT_RUNTIME_PATHS "${VTK_DIR}\\bin\\${_vtkConfig}")

        # libzeug
        list(APPEND PROJECT_RUNTIME_PATHS "${libzeug_DIR}\\bin")

        # Qt
        get_filename_component(Qt5QMake_DIR ${Qt5QMake_PATH} DIRECTORY)
        list(APPEND PROJECT_RUNTIME_PATHS "${Qt5QMake_DIR}")

        # plugins that require additional third party libraries may add their paths to PLUGIN_RUNTIME_PATHS
        if(PLUGIN_RUNTIME_PATHS)
            list(APPEND PROJECT_RUNTIME_PATHS ${PLUGIN_RUNTIME_PATHS})
        endif()

        set(MSVC_LOCAL_DEBUGGER_ENVIRONMENT_${UPPER_CONFIG_TYPE}
           "PATH=${PROJECT_RUNTIME_PATHS};%PATH%"
       )
    endforeach()

    configure_file(
        ${PROJECT_SOURCE_DIR}/cmake/project_config_templates/vcxproj.user.in
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.vcxproj.user
    )

endfunction()
