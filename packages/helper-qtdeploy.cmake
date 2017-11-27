
# copy required Qt binaries to deploy them with installs/packages

if (CMAKE_VERSION VERSION_LESS 3.0)
    message(WARNING "Automated Qt deployment is only available with CMake version >= 3.0.")
endif()

if (CMAKE_VERSION VERSION_LESS 3.0 OR NOT OPTION_INSTALL_3RDPARTY_BINARIES)

    function(deployQtBinariesForTarget TARGET_NAME)
    endfunction()

    return()

endif()


set(QT_DEPLOY_DIR ${PROJECT_BINARY_DIR}/qt_deploy)

if (WIN32)
    if (OpenGL IN_LIST PROJECT_QT_COMPONENTS)
        set(DeployQtOpenGL ON)
        # deprecated Qt5 OpenGL module is not deployed by Qt anymore
        get_target_property(QtOpenGL_release_location Qt5::OpenGL IMPORTED_LOCATION_RELEASE)
        get_target_property(QtOpenGL_debug_location   Qt5::OpenGL IMPORTED_LOCATION_DEBUG)
    endif()

    get_filename_component(_qtBinDir ${Qt5QMake_PATH} DIRECTORY)

    if(${Qt5Core_VERSION} VERSION_LESS 5.6)
        set(_windeployqtParams
            $<$<CONFIG:Debug>:--debug>$<$<CONFIG:RelWithDebInfo>:--release-with-debug-info>$<$<OR:$<CONFIG:Release>,$<CONFIG:RelNoOptimization>>:--release>
            --no-translations
            --no-compiler-runtime
            --no-system-d3d-compiler
        )
    else()
        set(_windeployqtParams
        # If conditions of generator expressions don't match, they are replaced by "",
        # which is then interpreted as an empty parameter.
        # Thus, $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:RelNoOptimization>>:--pdb>
        # resolves to "" for Release builds. So just don't use this flag for now.
            $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
            --no-translations
            --no-compiler-runtime
            --no-system-d3d-compiler
            --no-angle
            --no-opengl-sw
        )
    endif()

    add_custom_target(PrepareQtDeploy
        DEPENDS ${META_PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT_DEPLOY_DIR}"
        COMMAND ${_qtBinDir}/windeployqt
            "${PROJECT_BINARY_DIR}/$<CONFIG>/${META_PROJECT_NAME}$<$<CONFIG:Debug>:_d>$<$<CONFIG:RelWithDebInfo>:_rd>$<$<CONFIG:RelNoOptimization>:_rd0>.exe"
            --dir "${QT_DEPLOY_DIR}"
            ${_windeployqtParams}
        VERBATIM
    )
    if (DeployQtOpenGL)
        add_custom_command(TARGET PrepareQtDeploy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<$<NOT:$<CONFIG:Debug>>:${QtOpenGL_release_location}>$<$<CONFIG:DEBUG>:${QtOpenGL_debug_location}>"
                "${QT_DEPLOY_DIR}"
        )
    endif()
    set_target_properties(PrepareQtDeploy
        PROPERTIES
        FOLDER  "${IDE_FOLDER}"
    )

elseif(UNIX)

    # Putting UNIX deployment into a function to be able to safely abort it.
function(deploy_qt_for_unix)

    # Check Qt plugins first: if related CMake modules are not available, we should skip everything else here
    # NOTE: For Qt downloaded+installed from their website, the Qt5::Gui component lists all the
    # input, platform, etc. plugins. in Qt5Gui_PLUGINS. On the packaged Qt5 versions (e.g., in
    # Ubuntu 17.10), Qt5Gui_PLUGINS is empty, and no CMake modules are available that would
    # facilitate plugin deployment.
    # TODO: All that stuff could be manually deployed, too. But that's error-prone and hard to maintain.
    set(_allPlugins)
    foreach(_component ${PROJECT_QT_COMPONENTS})
        list(APPEND _allPlugins ${Qt5${_component}_PLUGINS})
    endforeach()

    if (NOT Qt5::QXcbGlxIntegrationPlugin IN_LIST _allPlugins)
        message(WARNING "Binary deployment if Qt5 plugins is not supported with your Qt \
installation. If you require a self-contained binary package, consider switching to a manual Qt \
installation downloaded from https://www1.qt.io/download-open-source/
See notes in helper-qtdeploy.cmake for details of this problem.
SKIPPING DEPLOYMENT OF Qt LIBRARIES AND PLUGINS!")
        return()
    endif()

    # qt.conf: setup deploy folder structure
    install(FILES qt.conf.linux
        DESTINATION ${INSTALL_BIN}
        RENAME qt.conf
    )

    function(getTruncatedFileName _longName _cutLength _resultVar)
        string(LENGTH ${_longName} _length)
        math(EXPR _length "${_length} - ${_cutLength}")
        string(SUBSTRING ${_longName} 0 ${_length} _truncated)
        get_filename_component(_truncated ${_truncated} NAME)
        set(${_resultVar} ${_truncated} PARENT_SCOPE)
    endfunction()

    # Libraries: need to rename, e.g., "libQt5Core.so.5.6.0" -> "libQt5Core.so.5"
    foreach(_component ${PROJECT_QT_COMPONENTS})
        get_target_property(_locationRelease Qt5::${_component} IMPORTED_LOCATION_RELEASE)
        get_target_property(_locationDebug Qt5::${_component} IMPORTED_LOCATION_DEBUG)

        if (NOT _locationDebug)
            set(_locationDebug ${_locationRelease})
        endif()

        getTruncatedFileName(${_locationRelease} 4 _locationReleaseTruncated)
        getTruncatedFileName(${_locationDebug} 4 _locationDebugTruncated)

        install(FILES ${_locationRelease}
            DESTINATION ${INSTALL_SHARED}
            CONFIGURATIONS Release RelWithDebInfo RelNoOptimization
            RENAME ${_locationReleaseTruncated}
        )
        install(FILES ${_locationDebug}
            DESTINATION ${INSTALL_SHARED}
            CONFIGURATIONS Debug
            RENAME ${_locationDebugTruncated}
        )
    endforeach()

    # Plugins: copy as is [can't check what is actually needed]
    foreach(_plugin ${_allPlugins})
        get_target_property(_locationRelease ${_plugin} IMPORTED_LOCATION_RELEASE)
        get_target_property(_locationDebug ${_plugin} IMPORTED_LOCATION_DEBUG)

        get_filename_component(_directory ${_locationRelease} DIRECTORY)
        get_filename_component(_pluginCategory ${_directory} NAME)

#         if (${_locationRelease} MATCHES "^.*/platforms/.*$")
            # only need to deploy this platform plugin. And it has to be located at a fixed position relative to the executable.
#             if (NOT _plugin STREQUAL "Qt5::QXcbIntegrationPlugin")
#                 continue()
#             endif()
#             set(_pluginInstallDir ${INSTALL_BIN}/platforms)
#         else()
#             set(_pluginInstallDir ${INSTALL_SHARED})
#         endif()

        if (NOT _locationDebug)
            set(_locationDebug ${_locationRelease})
        endif()

        install(FILES ${_locationRelease}
            DESTINATION ${INSTALL_SHARED}/${_pluginCategory}
            CONFIGURATIONS Release RelWithDebInfo RelNoOptimization
        )
        install(FILES ${_locationDebug}
            DESTINATION ${INSTALL_SHARED}/${_pluginCategory}
            CONFIGURATIONS Debug
        )
    endforeach()


    # ICU: no Qt-CMake support at all.
    # Find all ICU libs shipped with Qt (hopefully not something else..) and copy to shortened names.
    get_target_property(_qtCoreLibLocation Qt5::Core IMPORTED_LOCATION_RELEASE)
    get_filename_component(_qtLibDir ${_qtCoreLibLocation} DIRECTORY)
    file(GLOB _icuPaths
        LIST_DIRECTORIES false
        ${_qtLibDir}/*icu*)
    set(_realIcuPaths)
    foreach(_icuLib ${_icuPaths})
        get_filename_component(_realLibPath ${_icuLib} REALPATH)
        list(APPEND _realIcuPaths ${_realLibPath})
    endforeach()
    list(REMOVE_DUPLICATES _realIcuPaths)
    foreach(_icuLib ${_realIcuPaths})
        getTruncatedFileName(${_icuLib} 2 _icuLibTruncated)
        install(FILES ${_icuLib}
            DESTINATION ${INSTALL_SHARED}
            RENAME ${_icuLibTruncated}
        )
    endforeach()

    # Platform plugin: not listed in the CMakes' WTF??
    file(GLOB _XcbQpa_paths
        LIST_DIRECTORIES false
        ${_qtLibDir}/*XcbQpa*so*)
    if (NOT _XcbQpa_paths)
        message(FATAL_ERROR "Qt Platform plugin \"XcbQpa\" not found!")
    endif()
    set(_XcbQpa_path)
    foreach(_plugin ${_XcbQpa_paths})
        get_filename_component(_realPath ${_plugin} REALPATH)
        list(APPEND _XcbQpa_path ${_realPath})
    endforeach()
    list(REMOVE_DUPLICATES _XcbQpa_path)
    list(LENGTH _XcbQpa_path _num_XcbQpa_paths)
    if (NOT 1 EQUAL _num_XcbQpa_paths)
        message(FATAL_ERROR "Unexpected number of Qt Platform plugin libraries found (\"XcbQpa\", ${_num_XcbQpa_paths})")
    endif()
    getTruncatedFileName(${_XcbQpa_path} 4 _XcbQpa_pathTruncated)
    install(FILES ${_XcbQpa_path}
        DESTINATION ${INSTALL_SHARED}
        RENAME ${_XcbQpa_pathTruncated}
    )
endfunction()

    deploy_qt_for_unix()

endif()

function(deployQtBinariesForTarget TARGET_NAME)
    if (WIN32)
        add_dependencies(${TARGET_NAME} PrepareQtDeploy)
        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_SHARED})
    endif()
endfunction()
