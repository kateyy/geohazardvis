
# copy required Qt binaries to deploy them with installs/packages

function(deployQtBinariesForTarget TARGET_NAME)

    if(MSVC AND CPACK_INSTALL_3RDPARTY_DLLS)

        set(QT_DEPLOY_DIR ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_CONFIGURATION_TYPE}/qt_deploy)
        set(QT_DEPLOY_BUILD_TYPE "--release")
        if(${CMAKE_INSTALL_CONFIGURATION_TYPE} STREQUAL "Debug")
            set(EXE_POSTFIX "d")
            set(QT_DEPLOY_BUILD_TYPE "--debug")
        elseif(${CMAKE_INSTALL_CONFIGURATION_TYPE} STREQUAL "RelWithDebInfo")
            set(EXE_POSTFIX "rd")
        endif()
        set(EXE ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_CONFIGURATION_TYPE}/${META_PROJECT_NAME}${EXE_POSTFIX}.exe)
        
        add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
            COMMAND windeployqt "${EXE}" --dir "${QT_DEPLOY_DIR}" "${QT_DEPLOY_BUILD_TYPE}"
            VERBATIM)

        install(DIRECTORY "${QT_DEPLOY_DIR}/" DESTINATION ${INSTALL_BIN})
        
    endif()
    
endfunction()
