
set( TARGET ${META_PROJECT_NAME} )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp
    Application.h
    Application.cpp )
    
source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})

add_executable ( ${TARGET} ${SOURCES} )

target_link_libraries( ${TARGET}
    ${LIBZEUG_PROPERTYGUI_LIBRARY}
    ${LIBZEUG_REFLECTION_LIBRARY}
    ${LIBZEUG_SIGNAL_LIBRARY}
    Qt5::Widgets

    core
    gui )

configure_cxx_target( ${TARGET} )

install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
#   LIBRARY DESTINATION ${INSTALL_SHARED}
#   ARCHIVE DESTINATION ${INSTALL_LIB}
)
