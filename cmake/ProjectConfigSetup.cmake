

if(MSVC)
    option(OPTION_MSVC_AUTOMATIC_PROJECT_CONFIG
        "Automatically setup user specific MSVC project configuration (environment, working directory)"
        ON)
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

        set(PROJECT_PATHS)

        # VTK
        list(APPEND PROJECT_PATHS "${VTK_DIR}\\bin\\${CONFIG_TYPE}")

        # libzeug
        list(APPEND PROJECT_PATHS "${libzeug_DIR}\\bin")

        # TODO Qt

        set(PROJECT_PATH "")
        foreach(PATH_ENTRY ${PROJECT_PATHS})
            set(PROJECT_PATH "${PROJECT_PATH}${PATH_ENTRY};")
        endforeach()

        set(MSVC_LOCAL_DEBUGGER_ENVIRONMENT_${UPPER_CONFIG_TYPE}
           "PATH=${PROJECT_PATH}%PATH%"
       )
    endforeach()

    configure_file(
        ${PROJECT_SOURCE_DIR}/cmake/project_config_templates/vcxproj.user.in
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.vcxproj.user
    )

endfunction()
