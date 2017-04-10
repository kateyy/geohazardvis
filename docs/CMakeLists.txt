
# Target 'docs'
# add_custom_target(docs)
# set_target_properties(docs PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)

option(OPTION_BUILD_DOCUMENTATION "Build the source documentation (requires doxygen)." ON)
mark_as_advanced(OPTION_BUILD_DOCUMENTATION)

if (NOT OPTION_BUILD_DOCUMENTATION)
    return()
endif()

# Documentation
add_subdirectory(doxygen)
