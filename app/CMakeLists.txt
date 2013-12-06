
set( TARGET vtk_qt_testing )
message(STATUS ${TARGET})

set( SOURCES
    main.cpp )

add_executable ( ${TARGET} ${SOURCES} )

target_link_libraries( ${PROJECTNAME}
    ${VTK_LIBRARIES}
    gui)
    
set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX )
