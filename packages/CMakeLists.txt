
if (UNIX)
    install(PROGRAMS
        "GeohazardVis.sh"
        "GeohazardVis.desktop"
        DESTINATION ${INSTALL_ROOT}
    )
endif()


# Create output directory
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/packages)

include(helper-qtdeploy.cmake)

# Target 'pack'
# add_custom_target(pack)
# set_target_properties(pack PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

# Packages
include(pack.cmake)
