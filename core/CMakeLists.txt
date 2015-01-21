
set( TARGET core )
message( STATUS ${TARGET} )

set ( SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_api.h

    AbstractVisualizedData.h
    AbstractVisualizedData.cpp
    DataSetHandler.h
    DataSetHandler.cpp
    types.h
    vtkcamerahelper.h
    vtkcamerahelper.cpp
    vtkhelper.h
    common/ebem3d_common.h
    common/file_parser.h
    common/file_parser.cpp

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

    filters/CentroidAsScalarsFilter.h
    filters/CentroidAsScalarsFilter.cpp
    filters/ArrayRenameFilter.h
    filters/ArrayRenameFilter.cpp
    filters/NoiseImageSource.h
    filters/NoiseImageSource.cpp
    filters/vtkImageDataLIC2D.h
    filters/vtkImageDataLIC2D.cpp

    io/Loader.h
    io/Loader.cpp
    io/TextFileReader.h
    io/TextFileReader.cpp
    io/MatricesToVtk.h
    io/MatricesToVtk.cpp

    rendered_data/RenderedData.h
    rendered_data/RenderedData.cpp
    rendered_data/RenderedData3D.h
    rendered_data/RenderedData3D.cpp
    rendered_data/RenderedImageData.h
    rendered_data/RenderedImageData.cpp
    rendered_data/RenderedPolyData.h
    rendered_data/RenderedPolyData.cpp
    rendered_data/RenderedVectorGrid3D.h
    rendered_data/RenderedVectorGrid3D.cpp

    context2D_data/Context2DData.h
    context2D_data/Context2DData.cpp
    context2D_data/ImageProfileContextPlot.h
    context2D_data/ImageProfileContextPlot.cpp
    context2D_data/vtkContextItemCollection.h
    context2D_data/vtkContextItemCollection.cpp
    context2D_data/vtkPlotCollection.h
    context2D_data/vtkPlotCollection.cpp

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

    color_mapping/ColorMapping.h
    color_mapping/ColorMapping.cpp
    color_mapping/ColorMappingData.h
    color_mapping/ColorMappingData.hpp
    color_mapping/ColorMappingData.cpp
    color_mapping/ColorMappingRegistry.h
    color_mapping/ColorMappingRegistry.cpp
    color_mapping/DefaultColorMapping.h
    color_mapping/DefaultColorMapping.cpp
    color_mapping/AttributeArrayComponentMapping.h
    color_mapping/AttributeArrayComponentMapping.cpp
    color_mapping/VectorMagnitudeColorMapping.h
    color_mapping/VectorMagnitudeColorMapping.cpp
    color_mapping/VectorField3DLIC2DPlanes.h
    color_mapping/VectorField3DLIC2DPlanes.cpp
    color_mapping/VertexComponentColorMapping.h
    color_mapping/VertexComponentColorMapping.cpp
    color_mapping/GlyphColorMapping.h
    color_mapping/GlyphColorMapping.cpp
    color_mapping/GlyphColorMappingGlyphListener.h
    color_mapping/GlyphColorMappingGlyphListener.cpp
    color_mapping/GlyphMagnitudeColorMapping.h
    color_mapping/GlyphMagnitudeColorMapping.cpp

    glyph_mapping/GlyphMapping.h
    glyph_mapping/GlyphMapping.cpp
    glyph_mapping/GlyphMappingData.h
    glyph_mapping/GlyphMappingData.hpp
    glyph_mapping/GlyphMappingData.cpp
    glyph_mapping/GlyphMappingRegistry.h
    glyph_mapping/GlyphMappingRegistry.cpp
    glyph_mapping/Grid3dGlyphMapping.h
    glyph_mapping/Grid3dGlyphMapping.cpp
    glyph_mapping/PolyDataAttributeGlyphMapping.h
    glyph_mapping/PolyDataAttributeGlyphMapping.cpp
    
    reflectionzeug_extension/QStringProperty.h
    reflectionzeug_extension/QStringProperty.hpp
    reflectionzeug_extension/QStringProperty.cpp
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
