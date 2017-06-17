#include <gtest/gtest.h>

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>
#include <core/utility/vtkVector_print.h>


class GenericPolyDataObject_test : public ::testing::Test
{
public:
    static ReferencedCoordinateSystemSpecification defaultCoordsSpec()
    {
        ReferencedCoordinateSystemSpecification coordsSpec;
        coordsSpec.type = CoordinateSystemType::metricGlobal;
        coordsSpec.geographicSystem = "WGS 84";
        coordsSpec.globalMetricSystem = "UTM";
        coordsSpec.unitOfMeasurement = "m";
        coordsSpec.referencePointLatLong = { 60, 70 };
        coordsSpec.referencePointLocalRelative = { 0.5, 0.5 };
        return coordsSpec;
    }

    std::unique_ptr<PointCloudDataObject> genPointCloud(
        int numPoints = 6,
        const ReferencedCoordinateSystemSpecification & spec = defaultCoordsSpec())
    {
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetDataTypeToFloat();
        auto vertices = vtkSmartPointer<vtkCellArray>::New();
        vertices->InsertNextCell(numPoints);
        for (int i = 0; i < numPoints; ++i)
        {
            const double coord = static_cast<double>(i * 1000);
            points->InsertNextPoint(coord, coord, coord);
            vertices->InsertCellPoint(i);
        }
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        poly->SetPoints(points);
        poly->SetVerts(vertices);

        spec.writeToFieldData(*poly->GetFieldData());

        return std::make_unique<PointCloudDataObject>("PointCloud", *poly);
    }
};


TEST_F(GenericPolyDataObject_test, CorrectCoordinateSystem)
{
    auto pointCloud = genPointCloud();

    ASSERT_EQ(defaultCoordsSpec(), pointCloud->coordinateSystem());
}

TEST_F(GenericPolyDataObject_test, TransformCoords_PointCloud_m_to_km_metricGlobal)
{
    const int numPoints = 6;
    auto pointCloud = genPointCloud(numPoints);
    auto coordsM = vtkFloatArray::FastDownCast(pointCloud->polyDataSet().GetPoints()->GetData());

    auto specKm = defaultCoordsSpec();
    specKm.unitOfMeasurement = "km";

    ASSERT_TRUE(pointCloud->canTransformTo(specKm));

    auto inKmDataSet = pointCloud->coordinateTransformedDataSet(specKm);
    ASSERT_TRUE(inKmDataSet);

    auto inKm = vtkPolyData::SafeDownCast(inKmDataSet);
    ASSERT_TRUE(inKm);

    auto coordsKm = vtkFloatArray::FastDownCast(inKm->GetPoints()->GetData());
    ASSERT_TRUE(coordsKm);

    vtkVector3f pointM, pointKm;

    for (int i = 0; i < numPoints; ++i)
    {
        coordsM->GetTypedTuple(i, pointM.GetData());
        coordsKm->GetTypedTuple(i, pointKm.GetData());
        ASSERT_EQ(vtkVector3f(pointM * 0.001f), pointKm);
    }

    const auto specFromPipeline = ReferencedCoordinateSystemSpecification::fromFieldData(*inKm->GetFieldData());
    ASSERT_EQ(specKm, specFromPipeline);
}

TEST_F(GenericPolyDataObject_test, TransformCoords_PointCloud_m_to_km_metricLocal)
{
    auto spec = defaultCoordsSpec();
    spec.type = CoordinateSystemType::metricLocal;
    const int numPoints = 6;
    auto pointCloud = genPointCloud(numPoints, spec);
    auto coordsM = vtkFloatArray::FastDownCast(pointCloud->polyDataSet().GetPoints()->GetData());

    auto specKm = spec;
    specKm.unitOfMeasurement = "km";

    ASSERT_TRUE(pointCloud->canTransformTo(specKm));

    auto inKmDataSet = pointCloud->coordinateTransformedDataSet(specKm);
    ASSERT_TRUE(inKmDataSet);

    auto inKm = vtkPolyData::SafeDownCast(inKmDataSet);
    ASSERT_TRUE(inKm);

    auto coordsKm = vtkFloatArray::FastDownCast(inKm->GetPoints()->GetData());
    ASSERT_TRUE(coordsKm);

    vtkVector3f pointM, pointKm;

    for (int i = 0; i < numPoints; ++i)
    {
        coordsM->GetTypedTuple(i, pointM.GetData());
        coordsKm->GetTypedTuple(i, pointKm.GetData());
        ASSERT_EQ(vtkVector3f(pointM * 0.001f), pointKm);
    }

    const auto specFromPipeline = ReferencedCoordinateSystemSpecification::fromFieldData(*inKm->GetFieldData());
    ASSERT_EQ(specKm, specFromPipeline);
}

TEST_F(GenericPolyDataObject_test, TransformCoords_PointCloud_m_to_km_storedCoords_toLocal)
{
    auto specGlobalM = defaultCoordsSpec();
    specGlobalM.type = CoordinateSystemType::metricGlobal;
    specGlobalM.referencePointLatLong = { 40.0, 50.0 };
    specGlobalM.referencePointLocalRelative = { 0.5, 0.5 };
    const int numPoints = 6;
    auto pointCloud = genPointCloud(numPoints, specGlobalM);
    auto coordsGlobalM = vtkFloatArray::FastDownCast(pointCloud->polyDataSet().GetPoints()->GetData());

    auto shiftToLocalM = -pointCloud->bounds().convertTo<float>().center();
    shiftToLocalM[2] = 0.0;

    auto coordsLocalM = vtkSmartPointer<vtkFloatArray>::New();
    coordsLocalM->DeepCopy(coordsGlobalM);
    for (int i = 0; i < numPoints; ++i)
    {
        vtkVector3f point;
        coordsLocalM->GetTypedTuple(i, point.GetData());
        point += shiftToLocalM;
        coordsLocalM->SetTypedTuple(i, point.GetData());
    }

    auto specLocalM = specGlobalM;
    specLocalM.type = CoordinateSystemType::metricLocal;
    specLocalM.writeToInformation(*coordsLocalM->GetInformation());

    pointCloud->polyDataSet().GetPointData()->AddArray(coordsLocalM);

    auto specLocalKm = specLocalM;
    specLocalKm.unitOfMeasurement = "km";

    ASSERT_TRUE(pointCloud->canTransformTo(specLocalKm));

    auto localInKmDataSet = pointCloud->coordinateTransformedDataSet(specLocalKm);
    ASSERT_TRUE(localInKmDataSet);

    auto localInKm = vtkPolyData::SafeDownCast(localInKmDataSet);
    ASSERT_TRUE(localInKm);
    ASSERT_TRUE(localInKm->GetPoints() && localInKm->GetPoints()->GetData());
    auto coordsLocalKm = vtkFloatArray::FastDownCast(localInKm->GetPoints()->GetData());
    ASSERT_TRUE(coordsLocalKm);

    vtkVector3f pointLocalM, pointLocalKm;

    for (int i = 0; i < numPoints; ++i)
    {
        coordsLocalM->GetTypedTuple(i, pointLocalM.GetData());
        coordsLocalKm->GetTypedTuple(i, pointLocalKm.GetData());
        ASSERT_EQ(vtkVector3f(pointLocalM * 0.001f), pointLocalKm);
    }

    const auto specFromPipeline = ReferencedCoordinateSystemSpecification::fromFieldData(*localInKm->GetFieldData());
    ASSERT_EQ(specLocalKm, specFromPipeline);
}
