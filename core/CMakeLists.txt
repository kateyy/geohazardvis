
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
    LINKER_LANGUAGE CXX
    COMPILE_DEFINITIONS_DEBUG   "${DEFAULT_COMPILE_DEFS_DEBUG}"
    COMPILE_DEFINITIONS_RELEASE "${DEFAULT_COMPILE_DEFS_RELEASE}"
    LINK_FLAGS_DEBUG            "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE          "${DEFAULT_LINKER_FLAGS_RELEASE}" )
