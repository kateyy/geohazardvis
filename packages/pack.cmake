
if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

    if(WIN32)
        set(OPTION_PACK_GENERATOR "ZIP;NSIS" CACHE STRING "Package targets")
    else()
        set(OPTION_PACK_GENERATOR "TGZ" CACHE STRING "Package targets")
    endif()

    # Reset CPack configuration
    if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
        set(CPACK_IGNORE_FILES "")
        set(CPACK_INSTALLED_DIRECTORIES "")
        set(CPACK_SOURCE_IGNORE_FILES "")
        set(CPACK_SOURCE_INSTALLED_DIRECTORIES "")
        set(CPACK_STRIP_FILES "")
        set(CPACK_SOURCE_TOPLEVEL_TAG "")
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "")
    endif()

    # Find cpack executable
    get_filename_component(CPACK_PATH ${CMAKE_COMMAND} PATH)
    set(CPACK_COMMAND "${CPACK_PATH}/cpack")


    set(project_name ${META_PROJECT_NAME})
    set(project_root ${META_PROJECT_NAME})

    set(package_name            ${META_PROJECT_NAME})
    set(package_description     ${META_PROJECT_DESCRIPTION})
    set(package_vendor          "${META_AUTHOR_MAINTAINER} ${META_AUTHOR_MAINTAINER_EMAIL}")


    set(CMAKE_MODULE_PATH                   ${PROJECT_SOURCE_DIR}/packages/${project_name})


    set(CPACK_PACKAGE_NAME                  "${package_name}")
    set(CPACK_PACKAGE_VENDOR                "${package_vendor}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "${package_description}")
    set(CPACK_PACKAGE_VERSION               "${META_VERSION}")
    set(CPACK_PACKAGE_VERSION_MAJOR         "${META_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR         "${META_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH         "${META_VERSION_PATCH}")
    set(CPACK_RESOURCE_FILE_LICENSE         "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README          "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_WELCOME         "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_DESCRIPTION_FILE      "${PROJECT_SOURCE_DIR}/README.md")
    #set(CPACK_PACKAGE_ICON                  "${PROJECT_SOURCE_DIR}/packages/logo.bmp")
    set(CPACK_PACKAGE_RELOCATABLE           ON)


    if(WIN32 AND CPACK_PACKAGE_ICON)
        # NOTE: for using MUI (UN)WELCOME images we suggest to replace nsis defaults,
        # since there is currently no way to do so without manipulating the installer template (which we won't).
        # http://public.kitware.com/pipermail/cmake-developers/2013-January/006243.html

        # SO the following only works for the installer icon, not for the welcome image.

        # NSIS requires "\\" - escaped backslash to work properly. We probably won't rely on this feature,
        # so just replacing / with \\ manually.

        #file(TO_NATIVE_PATH "${CPACK_PACKAGE_ICON}" CPACK_PACKAGE_ICON)
        string(REGEX REPLACE "/" "\\\\\\\\" CPACK_PACKAGE_ICON "${CPACK_PACKAGE_ICON}")
    endif()

    if(X64)
        # http://public.kitware.com/Bug/view.php?id=9094
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    endif()
    #set(CPACK_NSIS_DISPLAY_NAME             "${package_name}-${META_VERSION}")
    #set(CPACK_NSIS_MUI_ICON    "${PROJECT_SOURCE_DIR}/packages/logo.ico")
    #set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/packages/logo.ico")


    # set(CPACK_PACKAGE_FILE_NAME "${package_name}-${CPACK_PACKAGE_VERSION}")

    set(CPACK_INSTALL_CMAKE_PROJECTS        "${CMAKE_BINARY_DIR};${project_root};ALL;/")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY     "${package_name}")
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY  "${package_name}")
    set(CPACK_INSTALL_PREFIX            ".")

    set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackConfig-${project_name}.cmake")
    set(CPACK_GENERATOR ${OPTION_PACK_GENERATOR})

    if(NOT WIN32)
        # Important: Must be set to install files to absolute path (e.g., /etc)
        # -> CPACK_[RPM_]PACKAGE_RELOCATABLE = OFF
        set(CPACK_SET_DESTDIR ON)
    endif()
    set(CPack_CMake_INCLUDED FALSE)
    include(CPack)
endif()


# Package target

set(TARGET_NAME pack-${project_name})

add_custom_target(
    ${TARGET_NAME}
    COMMAND ${CPACK_COMMAND} --config ${CMAKE_BINARY_DIR}/CPackConfig-${project_name}.cmake
        -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_target_properties(${TARGET_NAME}
    PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD  ON
    FOLDER                      "${IDE_FOLDER}"
)

deployQtBinariesForTarget(${TARGET_NAME})


# Dependencies

if(MSVC)
    add_dependencies(pack-${project_name} ALL_BUILD)
endif()
#add_dependencies(pack pack-${project_name})
