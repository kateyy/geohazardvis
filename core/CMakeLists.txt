
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
    vtkcamerahelper.h
    vtkcamerahelper.cpp
    vtkhelper.h
    common/ebem3d_common.h
    common/file_parser.h
    common/file_parser.cpp

    data_objects/DataObject.h
    data_objects/DataObject.cpp
    data_objects/ImageDataObject.h
    data_objects/ImageDataObject.cpp
    data_objects/PolyDataObject.h
    data_objects/PolyDataObject.cpp
    data_objects/VectorGrid3DDataObject.h
    data_objects/VectorGrid3DDataObject.cpp
    data_objects/RawVectorData.h
    data_objects/RawVectorData.cpp
    data_objects/RenderedData.h
    data_objects/RenderedData.cpp
    data_objects/RenderedImageData.h
    data_objects/RenderedImageData.cpp
    data_objects/RenderedPolyData.h
    data_objects/RenderedPolyData.cpp
    data_objects/RenderedVectorGrid3D.h
    data_objects/RenderedVectorGrid3D.cpp

    table_model/QVtkTableModel.h
    table_model/QVtkTableModel.cpp
    table_model/QVtkTableModelImage.h
    table_model/QVtkTableModelImage.cpp
    table_model/QVtkTableModelPolyData.h
    table_model/QVtkTableModelPolyData.cpp
    table_model/QVtkTableModelRawVector.h
    table_model/QVtkTableModelRawVector.cpp
    table_model/QVtkTableModelVectorGrid3D.h
    table_model/QVtkTableModelVectorGrid3D.cpp

    scalar_mapping/ScalarToColorMapping.h
    scalar_mapping/ScalarToColorMapping.cpp
    scalar_mapping/ScalarsForColorMapping.h
    scalar_mapping/ScalarsForColorMapping.hpp
    scalar_mapping/ScalarsForColorMapping.cpp
    scalar_mapping/ScalarsForColorMappingRegistry.h
    scalar_mapping/ScalarsForColorMappingRegistry.cpp
    scalar_mapping/AbstractArrayComponentMapping.h
    scalar_mapping/AbstractArrayComponentMapping.cpp
    scalar_mapping/AttributeArrayComponentMapping.h
    scalar_mapping/AttributeArrayComponentMapping.cpp
    scalar_mapping/RawArrayComponentMapping.h
    scalar_mapping/RawArrayComponentMapping.cpp
    scalar_mapping/DefaultColorMapping.h
    scalar_mapping/DefaultColorMapping.cpp
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
    vector_mapping/RawVectorMapping.h
    vector_mapping/RawVectorMapping.cpp
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
