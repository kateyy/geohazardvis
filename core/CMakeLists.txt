
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    ${TARGET}_api.h
    
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
    
    QVtkTableModel.h
    QVtkTableModel.cpp
    
    data_objects/DataObject.h
    data_objects/DataObject.cpp
    data_objects/ImageDataObject.h
    data_objects/ImageDataObject.cpp
    data_objects/PolyDataObject.h
    data_objects/PolyDataObject.cpp
    data_objects/RenderedData.h
    data_objects/RenderedData.cpp
    data_objects/RenderedImageData.h
    data_objects/RenderedImageData.cpp
    data_objects/RenderedPolyData.h
    data_objects/RenderedPolyData.cpp
    
    data_mapping/ScalarToColorMapping.h
    data_mapping/ScalarToColorMapping.cpp
    data_mapping/ScalarsForColorMapping.h
    data_mapping/ScalarsForColorMapping.hpp
    data_mapping/ScalarsForColorMapping.cpp
    data_mapping/ScalarsForColorMappingRegistry.h
    data_mapping/ScalarsForColorMappingRegistry.cpp
    data_mapping/DefaultColorMapping.h
    data_mapping/DefaultColorMapping.cpp
    data_mapping/CoordinateValueMapping.h
    data_mapping/CoordinateValueMapping.cpp
)

source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} "\\\\.cpp$|\\\\.c$|\\\\.h$|\\\\.hpp$|\\\\.ui$|\\\\.inl$"
    "Source Files" ${SOURCES})

if (OPTION_BUILD_STATIC)
    add_library(${TARGET} ${SOURCES})
else()
    add_library(${TARGET} SHARED ${SOURCES})
endif()

target_link_libraries( ${TARGET}
    ${VTK_LIBRARIES}
    Qt5::Core
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

generate_library_export_header(${TARGET})
