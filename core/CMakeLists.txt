
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    ${TARGET}_api.h
    
    DataSetHandler.h
    DataSetHandler.cpp
    Loader.h
    Loader.cpp
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
    data_objects/AttributeVectorData.h
    data_objects/AttributeVectorData.cpp
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
    
    scalar_mapping/ScalarToColorMapping.h
    scalar_mapping/ScalarToColorMapping.cpp
    scalar_mapping/ScalarsForColorMapping.h
    scalar_mapping/ScalarsForColorMapping.hpp
    scalar_mapping/ScalarsForColorMapping.cpp
    scalar_mapping/ScalarsForColorMappingRegistry.h
    scalar_mapping/ScalarsForColorMappingRegistry.cpp
    scalar_mapping/AttributeArrayComponentMapping.h
    scalar_mapping/AttributeArrayComponentMapping.cpp
    scalar_mapping/DefaultColorMapping.h
    scalar_mapping/DefaultColorMapping.cpp
    scalar_mapping/CoordinateValueMapping.h
    scalar_mapping/CoordinateValueMapping.cpp
    scalar_mapping/GridColorMapping.h
    scalar_mapping/GridColorMapping.cpp

    vector_mapping/VectorsToSurfaceMapping.h
    vector_mapping/VectorsToSurfaceMapping.cpp
    vector_mapping/VectorsForSurfaceMapping.h
    vector_mapping/VectorsForSurfaceMapping.hpp
    vector_mapping/VectorsForSurfaceMapping.cpp
    vector_mapping/VectorsForSurfaceMappingRegistry.h
    vector_mapping/VectorsForSurfaceMappingRegistry.cpp
    vector_mapping/AttributeVectorMapping.h
    vector_mapping/AttributeVectorMapping.cpp
    vector_mapping/CellDataVectorMapping.h
    vector_mapping/CellDataVectorMapping.cpp
    vector_mapping/SurfaceNormalMapping.h
    vector_mapping/SurfaceNormalMapping.cpp    
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
    ${LIBZEUG_SIGNAL_LIBRARY}
    ${LIBZEUG_REFLECTION_LIBRARY}
    Qt5::Core
    Qt5::Gui
)

include_directories(
    ${LIBZEUG_SIGNAL_INCLUDE_DIR}
    ${LIBZEUG_REFLECTION_INCLUDE_DIR}
)


configure_cxx_target( ${TARGET} )

generate_library_export_header(${TARGET})
