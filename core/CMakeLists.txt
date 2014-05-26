
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    Loader.h
    Loader.cpp
    Input.h
    Input.cpp
    TextFileReader.h
    TextFileReader.cpp
    vtkhelper.h
    common/ebem3d_common.h
    common/file_parser.h
    common/file_parser.cpp
)

source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.ui$|\\\\.inl$"
    "Source Files" ${SOURCES})

add_library ( ${TARGET}
    ${SOURCES} )

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
)

set_target_properties( ${TARGET}
    PROPERTIES
    LINKER_LANGUAGE                     CXX
    COMPILE_DEFINITIONS_DEBUG           "${DEFAULT_COMPILE_DEFS_DEBUG}"
    COMPILE_DEFINITIONS_RELEASE         "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_DEFINITIONS_RELWITHDEBINFO  "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_DEFINITIONS_MINSIZEREL      "${DEFAULT_COMPILE_DEFS_RELEASE}"
    COMPILE_FLAGS                       "${DEFAULT_COMPILE_FLAGS}"
    LINK_FLAGS_DEBUG                    "${DEFAULT_LINKER_FLAGS_DEBUG}"
    LINK_FLAGS_RELEASE                  "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_RELWITHDEBINFO           "${DEFAULT_LINKER_FLAGS_RELEASE}"
    LINK_FLAGS_MINSIZEREL               "${DEFAULT_LINKER_FLAGS_RELEASE}"
)
