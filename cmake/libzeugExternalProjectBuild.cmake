# https://cmake.org/pipermail/cmake/2011-July/045274.html

if(BUILD_CONFIG STREQUAL "Debug")
    set(LIBZEUG_BUILD_CONFIG "Debug")
else()
    set(LIBZEUG_BUILD_CONFIG "Release")
endif()

set(PARAMS 
    --build ${CMAKE_BINARY_DIR}
    --config ${LIBZEUG_BUILD_CONFIG}
)

execute_process(COMMAND ${CMAKE_COMMAND} ${PARAMS})

execute_process(COMMAND ${CMAKE_COMMAND} ${PARAMS} --target install)
