
# copy required Qt binaries to deploy them with installs/packages

set(QT_DEPLOY_DIR ${CMAKE_BINARY_DIR}/qt_deploy)

if(WIN32 AND ${CMAKE_MAJOR_VERSION} VERSION_GREATER 3)
    add_custom_target(PrepareQtDeploy
        DEPENDS ${META_PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT_DEPLOY_DIR}"
        COMMAND windeployqt "${CMAKE_BINARY_DIR}/$<CONFIG>/${META_PROJECT_NAME}$<$<CONFIG:Debug>:d>$<$<CONFIG:RelWithDebInfo>:rd>.exe" --dir "${QT_DEPLOY_DIR}" $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
        VERBATIM
    )
elseif(WIN32)
    # WIN32 and old CMake
    message(warning "Automated Qt is only implemented for CMake version >= 3.0!")
endif()

function(deployQtBinariesForTarget TARGET_NAME)

    if(WIN32 AND CPACK_INSTALL_3RDPARTY_DLLS AND ${CMAKE_MAJOR_VERSION} VERSION_GREATER 3)

        add_dependencies(${TARGET_NAME} PrepareQtDeploy)

        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_BIN})

    endif()

endfunction()
