
set( TARGET vtk_qt_testing )
message( STATUS ${TARGET} )

set ( SOURCES
    main.cpp )

add_executable ( ${TARGET} ${SOURCES} )

qt5_use_modules( ${TARGET} Widgets )

target_link_libraries( ${TARGET}
    core
    gui )
    
set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX )
