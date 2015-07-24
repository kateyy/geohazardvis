
set( TARGET ProbeData )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated" ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h)
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

add_executable ( ${TARGET} ${SOURCES} )

set_target_properties( ${TARGET} PROPERTIES
    FOLDER "${IDE_FOLDER}"
)

target_link_libraries( ${TARGET}
    # ${LIBZEUG_REFLECTION_LIBRARY}
    # ${LIBZEUG_SIGNAL_LIBRARY}
    ${LIBZEUG_LIBRARIES}
    ${VTK_LIBRARIES}
    # Qt5::Widgets
    
    core
    # gui
)


include_directories(
    ${LIBZEUG_INCLUDES}
)

configure_cxx_target( ${TARGET} )
