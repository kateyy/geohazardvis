
# ExternalProjectMapBuild
#
# Map project's (custom) build configurations to ones that are available third party packages.
# For simplicity, use Release for all non-"Debug" builds.
#
# https://cmake.org/pipermail/cmake/2011-July/045274.html
#

if(BUILD_CONFIG STREQUAL "Debug")
    set(_package_config "Debug")
else()
    set(_package_config "Release")
endif()

set(PARAMS 
    --build ${CMAKE_BINARY_DIR}
    --config ${_package_config}
)

execute_process(COMMAND ${CMAKE_COMMAND} ${PARAMS})

execute_process(COMMAND ${CMAKE_COMMAND} ${PARAMS} --target install)
