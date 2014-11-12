
set( TARGET testing )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp
)

add_executable ( ${TARGET} ${SOURCES} )

target_link_libraries( ${TARGET}
    ${LIBZEUG_SIGNAL_LIBRARY}
    ${LIBZEUG_REFLECTION_LIBRARY}
    ${LIBZEUG_PROPERTYGUI_LIBRARY}
    # Qt5::Widgets
    
    ${VTK_LIBRARIES}
    
    core
    # gui
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBZEUG_SIGNAL_INCLUDE_DIR}
    ${LIBZEUG_REFLECTION_INCLUDE_DIR}
    ${LIBZEUG_PROPERTYGUI_INCLUDE_DIR}
)

configure_cxx_target( ${TARGET} )
