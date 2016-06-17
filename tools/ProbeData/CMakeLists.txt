
set(target ProbeData)
message(STATUS ${target})

set(sources
    main.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated" ${CMAKE_CURRENT_BINARY_DIR}/${target}_api.h)
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

add_executable(${target} ${sources})

target_link_libraries(${target}
    # ${LIBZEUG_REFLECTION_LIBRARY}
    # ${LIBZEUG_SIGNAL_LIBRARY}
    ${LIBZEUG_LIBRARIES}
    ${VTK_LIBRARIES}
    # Qt5::Widgets

    core
    # gui
)

target_include_directories(${target} SYSTEM
    PUBLIC ${LIBZEUG_INCLUDES}
)

configure_cxx_target(${target})
setupProjectUserConfig(${target})
