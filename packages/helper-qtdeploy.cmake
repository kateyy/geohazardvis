
# copy required Qt binaries to deploy them with installs/packages

set(QT_DEPLOY_DIR ${CMAKE_BINARY_DIR}/qt_deploy)

if(WIN32 AND ${CMAKE_VERSION} VERSION_GREATER 3.0)

    # deprecated Qt5 OpenGL module is not deployed by qt anymore
    get_target_property(QtOpenGL_release_location Qt5::OpenGL IMPORTED_LOCATION_RELEASE)
    get_target_property(QtOpenGL_debug_location   Qt5::OpenGL IMPORTED_LOCATION_DEBUG)

    add_custom_target(PrepareQtDeploy
        DEPENDS ${META_PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT_DEPLOY_DIR}"
        COMMAND windeployqt
            "${CMAKE_BINARY_DIR}/$<CONFIG>/${META_PROJECT_NAME}$<$<CONFIG:Debug>:_d>$<$<CONFIG:RelWithDebInfo>:_rd>$<$<CONFIG:RelNoOptimization>:_rd0>.exe"
            --dir "${QT_DEPLOY_DIR}"
            $<$<CONFIG:Debug>:--debug>$<$<CONFIG:RelWithDebInfo>:--release-with-debug-info>$<$<OR:$<CONFIG:Release>,$<CONFIG:RelNoOptimization>>:--release>
            --no-translations
            --no-compiler-runtime
            --no-system-d3d-compiler
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<$<NOT:$<CONFIG:Debug>>:${QtOpenGL_release_location}>$<$<CONFIG:DEBUG>:${QtOpenGL_debug_location}>"
            "${QT_DEPLOY_DIR}"
        VERBATIM
    )
    set_target_properties(PrepareQtDeploy
        PROPERTIES
        FOLDER  "${IDE_FOLDER}"
    )
elseif(WIN32)
    # WIN32 and old CMake
    message(WARNING "Automated Qt is only implemented for CMake version >= 3.0!")
endif()

function(deployQtBinariesForTarget TARGET_NAME)

    if(WIN32 AND CPACK_INSTALL_3RDPARTY_DLLS AND ${CMAKE_VERSION} VERSION_GREATER 3.0)

        add_dependencies(${TARGET_NAME} PrepareQtDeploy)

        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_BIN})

    endif()

endfunction()
