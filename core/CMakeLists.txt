
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h

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
    data_objects/DataObject_private.h
    data_objects/DataObject_private.cpp
    data_objects/ImageDataObject.h
    data_objects/ImageDataObject.cpp
    data_objects/ImageProfileData.h
    data_objects/ImageProfileData.cpp
    data_objects/PolyDataObject.h
    data_objects/PolyDataObject.cpp
    data_objects/VectorGrid3DDataObject.h
    data_objects/VectorGrid3DDataObject.cpp
    data_objects/RawVectorData.h
    data_objects/RawVectorData.cpp
    data_objects/RenderedData.h
    data_objects/RenderedData.cpp
    data_objects/RenderedData3D.h
    data_objects/RenderedData3D.cpp
    data_objects/ImageProfilePlot.h
    data_objects/ImageProfilePlot.cpp
    data_objects/RenderedImageData.h
    data_objects/RenderedImageData.cpp
    data_objects/RenderedPolyData.h
    data_objects/RenderedPolyData.cpp
    data_objects/RenderedVectorGrid3D.h
    data_objects/RenderedVectorGrid3D.cpp

    filters/CentroidAsScalarsFilter.h
    filters/CentroidAsScalarsFilter.cpp

    canvas_export/CanvasExporter.h
    canvas_export/CanvasExporter.cpp
    canvas_export/CanvasExporterRegistry.h
    canvas_export/CanvasExporterRegistry.cpp
    canvas_export/CanvasExporterImages.h
    canvas_export/CanvasExporterImages.cpp
    canvas_export/CanvasExporterPNG.h
    canvas_export/CanvasExporterPNG.cpp
    canvas_export/CanvasExporterPS.h
    canvas_export/CanvasExporterPS.cpp
    canvas_export/CanvasExporterTIFF.h
    canvas_export/CanvasExporterTIFF.cpp

    table_model/QVtkTableModel.h
    table_model/QVtkTableModel.cpp
    table_model/QVtkTableModelImage.h
    table_model/QVtkTableModelImage.cpp
    table_model/QVtkTableModelPolyData.h
    table_model/QVtkTableModelPolyData.cpp
    table_model/QVtkTableModelProfileData.h
    table_model/QVtkTableModelProfileData.cpp
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
    scalar_mapping/DefaultColorMapping.h
    scalar_mapping/DefaultColorMapping.cpp
    scalar_mapping/VertexComponentMapping.h
    scalar_mapping/VertexComponentMapping.cpp
    scalar_mapping/AbstractArrayComponentMapping.h
    scalar_mapping/AbstractArrayComponentMapping.cpp
    scalar_mapping/AttributeArrayComponentMapping.h
    scalar_mapping/AttributeArrayComponentMapping.cpp

    vector_mapping/VectorMapping.h
    vector_mapping/VectorMapping.cpp
    vector_mapping/VectorMappingData.h
    vector_mapping/VectorMappingData.hpp
    vector_mapping/VectorMappingData.cpp
    vector_mapping/VectorMappingRegistry.h
    vector_mapping/VectorMappingRegistry.cpp
    vector_mapping/Grid3dVectorMapping.h
    vector_mapping/Grid3dVectorMapping.cpp
    vector_mapping/PolyDataAttributeVectorMapping.h
    vector_mapping/PolyDataAttributeVectorMapping.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated" ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h)
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

if (OPTION_BUILD_STATIC)
    add_library(${TARGET} ${SOURCES})
else()
    add_library(${TARGET} SHARED ${SOURCES})
endif()

target_link_libraries( ${TARGET}
    ${LIBZEUG_REFLECTION_LIBRARY}
    ${LIBZEUG_SIGNAL_LIBRARY}
    ${VTK_LIBRARIES}
    Qt5::Core
)

include_directories(
    ${LIBZEUG_SIGNAL_INCLUDE_DIR}
    ${LIBZEUG_REFLECTION_INCLUDE_DIR}
)


configure_cxx_target( ${TARGET} )

generate_library_export_header(${TARGET})


install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)
