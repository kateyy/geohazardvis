
set(target core)
message(STATUS ${target})

set(sources
    ${CMAKE_CURRENT_BINARY_DIR}/${target}_api.h
    ${CONFIG_FILE}

    AbstractVisualizedData.h
    AbstractVisualizedData.cpp
    AbstractVisualizedData_private.h
    AbstractVisualizedData_private.cpp
    ApplicationSettings.h
    ApplicationSettings.cpp
    CoordinateSystems.h
    CoordinateSystems_fwd.h
    CoordinateSystems.cpp
    DataSetHandler.h
    DataSetHandler.cpp
    RuntimeInfo.h
    RuntimeInfo.cpp
    OpenGLDriverFeatures.h
    OpenGLDriverFeatures.cpp
    TemporalPipelineMediator.h
    TemporalPipelineMediator.cpp
    types.h
    types.cpp
    VersionInfo.h
    VersionInfo.cpp
    vtkCommandExt.h

    canvas_export/CanvasExporter.h
    canvas_export/CanvasExporter.cpp
    canvas_export/CanvasExporterImages.h
    canvas_export/CanvasExporterImages.cpp
    canvas_export/CanvasExporterJPEG.h
    canvas_export/CanvasExporterJPEG.cpp
    canvas_export/CanvasExporterPNG.h
    canvas_export/CanvasExporterPNG.cpp
    canvas_export/CanvasExporterPS.h
    canvas_export/CanvasExporterPS.cpp
    canvas_export/CanvasExporterRegistry.h
    canvas_export/CanvasExporterRegistry.cpp
    canvas_export/CanvasExporterTIFF.h
    canvas_export/CanvasExporterTIFF.cpp

    color_mapping/AttributeArrayComponentMapping.h
    color_mapping/AttributeArrayComponentMapping.cpp
    color_mapping/CentroidColorMapping.h
    color_mapping/CentroidColorMapping.cpp
    color_mapping/ColorBarRepresentation.h
    color_mapping/ColorBarRepresentation.cpp
    color_mapping/ColorMapping.h
    color_mapping/ColorMapping.cpp
    color_mapping/ColorMappingData.h
    color_mapping/ColorMappingData.cpp
    color_mapping/ColorMappingRegistry.h
    color_mapping/ColorMappingRegistry.cpp
    color_mapping/DefaultColorMapping.h
    color_mapping/DefaultColorMapping.cpp
    color_mapping/DirectImageColors.h
    color_mapping/DirectImageColors.cpp
    color_mapping/GlyphColorMapping.h
    color_mapping/GlyphColorMapping.cpp
    color_mapping/GlyphColorMappingGlyphListener.h
    color_mapping/GlyphColorMappingGlyphListener.cpp
    color_mapping/GlyphMagnitudeColorMapping.h
    color_mapping/GlyphMagnitudeColorMapping.cpp
    color_mapping/GradientResourceManager.h
    color_mapping/GradientResourceManager.cpp
    color_mapping/PointCoordinateColorMapping.h
    color_mapping/PointCoordinateColorMapping.cpp
    color_mapping/SlopeAngleMapping.h
    color_mapping/SlopeAngleMapping.cpp
    color_mapping/VectorMagnitudeColorMapping.h
    color_mapping/VectorMagnitudeColorMapping.cpp

    context2D_data/Context2DData.h
    context2D_data/Context2DData.cpp
    context2D_data/DataProfile2DContextPlot.h
    context2D_data/DataProfile2DContextPlot.cpp
    context2D_data/PlotPointsAndLine.h
    context2D_data/PlotPointsAndLine.cpp
    context2D_data/vtkContextItemCollection.h
    context2D_data/vtkContextItemCollection.cpp
    context2D_data/vtkPlotCollection.h
    context2D_data/vtkPlotCollection.cpp

    data_objects/CoordinateTransformableDataObject.h
    data_objects/CoordinateTransformableDataObject.cpp
    data_objects/DataObject.h
    data_objects/DataObject.hpp
    data_objects/DataObject.cpp
    data_objects/DataObject_private.h
    data_objects/DataObject_private.cpp
    data_objects/DataProfile2DDataObject.h
    data_objects/DataProfile2DDataObject.cpp
    data_objects/GenericPolyDataObject.h
    data_objects/GenericPolyDataObject.cpp
    data_objects/ImageDataObject.h
    data_objects/ImageDataObject.cpp
    data_objects/PointCloudDataObject.h
    data_objects/PointCloudDataObject.cpp
    data_objects/PolyDataObject.h
    data_objects/PolyDataObject.cpp
    data_objects/RawVectorData.h
    data_objects/RawVectorData.cpp
    data_objects/VectorGrid3DDataObject.h
    data_objects/VectorGrid3DDataObject.cpp

    filters/ArrayChangeInformationFilter.h
    filters/ArrayChangeInformationFilter.cpp
    filters/AssignPointAttributeToCoordinatesFilter.h
    filters/AssignPointAttributeToCoordinatesFilter.cpp
    filters/AttributeArrayModifiedListener.h
    filters/AttributeArrayModifiedListener.cpp
    filters/CentroidAsScalarsFilter.h
    filters/CentroidAsScalarsFilter.cpp
    filters/DEMApplyShadingToColors.h
    filters/DEMApplyShadingToColors.cpp
    filters/DEMImageNormals.h
    filters/DEMImageNormals.cpp
    filters/DEMShadingFilter.h
    filters/DEMShadingFilter.cpp
    filters/DEMToTopographyMesh.h
    filters/DEMToTopographyMesh.cpp
    filters/ExtractTimeStep.h
    filters/ExtractTimeStep.cpp
    filters/GeographicTransformationFilter.h
    filters/GeographicTransformationFilter.cpp
    filters/ImageBlankNonFiniteValuesFilter.h
    filters/ImageBlankNonFiniteValuesFilter.cpp
    filters/ImageMapToColors.h
    filters/ImageMapToColors.cpp
    filters/ImagePlaneWidget.h
    filters/ImagePlaneWidget.cpp
    filters/LineOnCellsSelector2D.h
    filters/LineOnCellsSelector2D.cpp
    filters/LineOnPointsSelector2D.h
    filters/LineOnPointsSelector2D.cpp
    filters/SetCoordinateSystemInformationFilter.h
    filters/SetCoordinateSystemInformationFilter.cpp
    filters/SetMaskedPointScalarsToNaNFilter.h
    filters/SetMaskedPointScalarsToNaNFilter.cpp
    filters/TemporalDataSource.h
    filters/TemporalDataSource.cpp
    filters/TemporalDifferenceFilter.h
    filters/TemporalDifferenceFilter.cpp
    filters/vtkInformationDoubleVectorMetaDataKey.h
    filters/vtkInformationDoubleVectorMetaDataKey.cpp
    filters/vtkInformationIntegerMetaDataKey.h
    filters/vtkInformationIntegerMetaDataKey.cpp
    filters/vtkInformationStringMetaDataKey.h
    filters/vtkInformationStringMetaDataKey.cpp

    glyph_mapping/GlyphMapping.h
    glyph_mapping/GlyphMapping.cpp
    glyph_mapping/GlyphMappingData.h
    glyph_mapping/GlyphMappingData.hpp
    glyph_mapping/GlyphMappingData.cpp
    glyph_mapping/GlyphMappingRegistry.h
    glyph_mapping/GlyphMappingRegistry.cpp
    glyph_mapping/Grid3dGlyphMapping.h
    glyph_mapping/Grid3dGlyphMapping.cpp
    glyph_mapping/PointCloudDataAttributeGlyphMapping.h
    glyph_mapping/PointCloudDataAttributeGlyphMapping.cpp
    glyph_mapping/PolyDataAttributeGlyphMapping.h
    glyph_mapping/PolyDataAttributeGlyphMapping.cpp

    io/BinaryFile.h
    io/BinaryFile.hpp
    io/BinaryFile.cpp
    io/DeformationTimeSeriesTextFileReader.h
    io/DeformationTimeSeriesTextFileReader.cpp
    io/Exporter.h
    io/Exporter.cpp
    io/io_helper.h
    io/io_helper.cpp
    io/Loader.h
    io/Loader.hpp
    io/Loader.cpp
    io/MetaTextFileReader.h
    io/MetaTextFileReader.cpp
    io/MatricesToVtk.h
    io/MatricesToVtk.cpp
    io/TextFileReader.h
    io/TextFileReader.cpp
    io/TextFileWriter.h
    io/TextFileWriter.cpp
    io/types.h

    reflectionzeug_extension/QStringProperty.h
    reflectionzeug_extension/QStringProperty.hpp
    reflectionzeug_extension/QStringProperty.cpp

    rendered_data/RenderedData.h
    rendered_data/RenderedData.cpp
    rendered_data/RenderedData_private.h
    rendered_data/RenderedData3D.h
    rendered_data/RenderedData3D.cpp
    rendered_data/RenderedImageData.h
    rendered_data/RenderedImageData.cpp
    rendered_data/RenderedPointCloudData.h
    rendered_data/RenderedPointCloudData.cpp
    rendered_data/RenderedPolyData.h
    rendered_data/RenderedPolyData.cpp
    rendered_data/RenderedVectorGrid3D.h
    rendered_data/RenderedVectorGrid3D.cpp

    table_model/QVtkTableModel.h
    table_model/QVtkTableModel.cpp
    table_model/QVtkTableModelImage.h
    table_model/QVtkTableModelImage.cpp
    table_model/QVtkTableModelPointCloudData.h
    table_model/QVtkTableModelPointCloudData.cpp
    table_model/QVtkTableModelPolyData.h
    table_model/QVtkTableModelPolyData.cpp
    table_model/QVtkTableModelProfileData.h
    table_model/QVtkTableModelProfileData.cpp
    table_model/QVtkTableModelRawVector.h
    table_model/QVtkTableModelRawVector.cpp
    table_model/QVtkTableModelVectorGrid3D.h
    table_model/QVtkTableModelVectorGrid3D.cpp

    utility/conversions.h
    utility/conversions.hpp
    utility/DataExtent.h
    utility/DataExtent.hpp
    utility/DataExtent_fwd.h
    utility/DataExtent_print.h
    utility/DataExtent_print.hpp
    utility/DataSetFilter.h
    utility/DataSetFilter.cpp
    utility/DataSetResidualHelper.h
    utility/DataSetResidualHelper.cpp
    utility/font.h
    utility/font.cpp
    utility/GeographicTransformationUtil.h
    utility/GeographicTransformationUtil.cpp
    utility/GridAxes3DActor.h
    utility/GridAxes3DActor.cpp
    utility/InterpolationHelper.h
    utility/InterpolationHelper.cpp
    utility/qthelper.h
    utility/qthelper.cpp
    utility/macros.h
    utility/mathhelper.h
    utility/mathhelper.hpp
    utility/mathhelper.cpp
    utility/memory.h
    utility/memory.hpp
    utility/type_traits.h
    utility/types_utils.h
    utility/types_utils.cpp
    utility/vtkcamerahelper.h
    utility/vtkcamerahelper.cpp
    utility/vtkCameraSynchronization.h
    utility/vtkCameraSynchronization.cpp
    utility/vtkpipelinehelper.h
    utility/vtkpipelinehelper.cpp
    utility/vtkstringhelper.h
    utility/vtkstringhelper.cpp
    utility/vtkvarianthelper.h
    utility/vtkvarianthelper.hpp
    utility/vtkvarianthelper.cpp
    utility/vtkVector_print.h
    utility/vtkVector_print.cpp
    utility/vtkvectorhelper.h
    utility/vtkvectorhelper.hpp
)

set(thirdPartySources
    ThirdParty/alphanum.hpp
    ThirdParty/dark_fusion_style.hpp
    ThirdParty/ParaView/vtkBoundingRectContextDevice2D.h
    ThirdParty/ParaView/vtkBoundingRectContextDevice2D.cxx
    ThirdParty/ParaView/vtkContext2DScalarBarActor.h
    ThirdParty/ParaView/vtkContext2DScalarBarActor.cxx
    ThirdParty/ParaView/vtkGridAxes2DActor.h
    ThirdParty/ParaView/vtkGridAxes2DActor.cxx
    ThirdParty/ParaView/vtkGridAxes3DActor.h
    ThirdParty/ParaView/vtkGridAxes3DActor.cxx
    ThirdParty/ParaView/vtkGridAxesHelper.h
    ThirdParty/ParaView/vtkGridAxesHelper.cxx
    ThirdParty/ParaView/vtkGridAxesPlane2DActor.h
    ThirdParty/ParaView/vtkGridAxesPlane2DActor.cxx
    ThirdParty/ParaView/vtkPVScalarBarRepresentation.h
    ThirdParty/ParaView/vtkPVScalarBarRepresentation.cxx
    ${PROJ4_INCLUDE_FILE}
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    # Rely on upstream warning fixes
    set_property(SOURCE ${thirdPartySources}
        PROPERTY COMPILE_FLAGS "-w"
    )
endif()

list(APPEND sources ${thirdPartySources})
list(APPEND sources ${THIRD_PARTY_INFO_HEADER})

if (OPTION_ENABLE_LIC2D)
    list(APPEND sources
        color_mapping/ImageDataLIC2DMapping.h
        color_mapping/ImageDataLIC2DMapping.cpp
        filters/NoiseImageSource.h
        filters/NoiseImageSource.cpp
        filters/StackedImageDataLIC3D.h
        filters/StackedImageDataLIC3D.cpp
    )
endif()

if (OPTION_ENABLE_TEXTURING)
    list(APPEND sources
        TextureManager.h
        TextureManager.cpp
        filters/TransformTextureCoords.h
        filters/TransformTextureCoords.cxx
    )
endif()


source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})
source_group_by_path(${CMAKE_CURRENT_BINARY_DIR} ".*" "Generated"
    ${CMAKE_CURRENT_BINARY_DIR}/${target}_api.h
    ${CONFIG_FILE}
    ${PROJ4_INCLUDE_FILE}
    ${THIRD_PARTY_INFO_HEADER}
)
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

add_library(${target} SHARED ${sources})

if(TARGET libzeug)
    add_dependencies(${target} libzeug)
endif()

target_include_directories(${target}
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
)

target_include_directories(${target} SYSTEM
    PUBLIC
        ${Qt5Core_INCLUDE_DIRS} # disable warnings produced by Qt headers (GCC)
        ${VTK_INCLUDE_DIRS}
        ${LIBZEUG_INCLUDES}
    PRIVATE
        ${PROJ4_INCLUDE_DIR}    # Don't include proj.4 in public headers!
)

target_link_libraries(${target}
    PUBLIC
        ${VTK_LIBRARIES}
        ${LIBZEUG_LIBRARIES}
        Qt5::Core
        ${PROJ4_LIBRARIES}
)

configure_cxx_target(${target})
writeBinaryVersionInfo(${target})

generate_library_export_header(${target})


install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN}
    LIBRARY DESTINATION ${INSTALL_SHARED}
    # ARCHIVE DESTINATION ${INSTALL_LIB}
)
