
set(target core-test)
message(STATUS "Test ${target}")


set(sources
    ${TEST_MAIN_CPP_FILEPATH}
    AbstractVisualizedData_test.cpp
    DataSetHandler_test.cpp
    VTKAlgorithmImplementations_test.cpp
    color_mapping/ColorMapping_test.cpp
    color_mapping/GlyphColorMapping_test.cpp
    context2D_data/DataProfile2DContextPlot_test.cpp
    data_objects/CoordinateTransformableDataObject_test.cpp
    data_objects/DataObject_test.cpp
    data_objects/GenericPolyDataObject_test.cpp
    data_objects/ImageDataObject_test.cpp
    filters/ArrayChangeInformationFilter_test.cpp
    filters/AssignPointAttributeToCoordinatesFilter_test.cpp
    filters/DEMImageNormals_test.cpp
    filters/DEMToTopographyMesh_test.cpp
    filters/PipelineInformationHelper.cpp
    filters/PipelineInformationHelper.h
    filters/SimpleImageGeoCoordinateTransformFilter_test.cpp
    io/BinaryFile_test.cpp
    io/DeformationTimeSeriesTextFileReader_test.cpp
    io/MatricesToVtk_test.cpp
    io/TextFileReader_test.cpp
    rendered_data/RenderedData_test.cpp
    table_model/QVtkTableModel_test.cpp
    utility/DataExtent_test.cpp
    utility/DataSetFilter_test.cpp
)

source_group_by_path_and_type(${CMAKE_CURRENT_SOURCE_DIR} ${sources})
source_group_by_path(${CMAKE_CURRENT_SOURCE_DIR} ".*" "" "CMakeLists.txt")

add_executable(${target} ${sources})

target_include_directories(${target} SYSTEM
    PUBLIC
        ${GTEST_INCLUDE_DIR}
})

target_link_libraries(${target}
    PUBLIC
        core
        ${GTEST_LIBRARIES}

    PRIVATE
        TestUtilities
)

configure_cxx_target(${target} NO_CPPCHECK)
setupProjectUserConfig(${target} GTEST_TARGET)
