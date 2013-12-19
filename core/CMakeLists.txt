
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    datagenerator.cpp
    datagenerator.h )


source_group("Source Files" FILES
    ${SOURCES}
)

add_library ( ${TARGET}
    ${SOURCES} )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
)

set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE CXX )
