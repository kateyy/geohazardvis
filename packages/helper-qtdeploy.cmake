
# copy required Qt binaries to deploy them with installs/packages

function(deployQtBinariesForTarget TARGET_NAME)

    if(MSVC AND CPACK_INSTALL_3RDPARTY_DLLS)

        set(QT_DEPLOY_DIR ${CMAKE_BINARY_DIR}/qt_deploy)

        set(EXE ${CMAKE_BINARY_DIR}/$<CONFIG>/${META_PROJECT_NAME}$<$<CONFIG:Debug>:d>$<$<CONFIG:RelWithDebInfo>:rd>.exe)
        
        add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} ARGS -E remove_directory "${QT_DEPLOY_DIR}"
            COMMAND windeployqt ARGS "${EXE}" --dir "${QT_DEPLOY_DIR}" $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
            VERBATIM)

        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_BIN})
        
    endif()
    
endfunction()
