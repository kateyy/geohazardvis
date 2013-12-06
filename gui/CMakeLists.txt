
set( TARGET gui )
message( STATUS ${TARGET} )

set ( SOURCES
    viewer.cpp
    viewer.h )

add_library ( ${TARGET} SHARED ${SOURCES} )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES} )


set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX )
