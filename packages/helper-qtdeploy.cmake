
# copy required Qt binaries to deploy them with installs/packages

set(QT_DEPLOY_DIR ${CMAKE_BINARY_DIR}/qt_deploy)

add_custom_target(PrepareQtDeploy
    DEPENDS ${META_PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${QT_DEPLOY_DIR}"
    COMMAND windeployqt "${CMAKE_BINARY_DIR}/$<CONFIG>/${META_PROJECT_NAME}$<$<CONFIG:Debug>:d>$<$<CONFIG:RelWithDebInfo>:rd>.exe" --dir "${QT_DEPLOY_DIR}" $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
    VERBATIM
)

function(deployQtBinariesForTarget TARGET_NAME)

    if(MSVC AND CPACK_INSTALL_3RDPARTY_DLLS)

        add_dependencies(${TARGET_NAME} PrepareQtDeploy)

        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_BIN})

    endif()

endfunction()
