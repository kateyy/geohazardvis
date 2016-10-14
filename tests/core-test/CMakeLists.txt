
set(target core-test)
message(STATUS "Test ${target}")


set(sources
    main.cpp
    ArrayChangeInformationFilter_test.cpp
    BinaryFile_test.cpp
    CoordinateTransformableDataObject_test.cpp
    DataExtent_test.cpp
    DataObject_test.cpp
    DataSetFilter_test.cpp
    DataSetHandler_test.cpp
    DEMImageNormals_test.cpp
    DEMToTopographyMesh_test.cpp
    DeformationTimeSeriesTextFileReader_test.cpp
    PipelineInformationHelper.h
    PipelineInformationHelper.cpp
    QVtkTableModel_test.cpp
    RenderedData_test.cpp
    SimpleImageGeoCoordinateTransformFilter_test.cpp
    TextFileReader_test.cpp
    VTKAlgorithmImplementations_test.cpp
)

add_executable(${target} ${sources})

target_link_libraries(${target}
    core
    ${GTEST_LIBRARIES}
    TestEnvironment
)

target_include_directories(${target}
    PRIVATE ${TestEnvIncludeDir}
)

configure_cxx_target(${target} NO_CPPCHECK)
setupProjectUserConfig(${target} GTEST_TARGET)
