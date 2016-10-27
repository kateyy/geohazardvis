#include <gtest/gtest.h>

#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <core/CoordinateSystems.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/utility/DataExtent.h>


class ImageDataObject_test : public testing::Test
{
public:
    static ReferencedCoordinateSystemSpecification defaultCoordsSpec()
    {
        ReferencedCoordinateSystemSpecification coordsSpec;
        coordsSpec.type = CoordinateSystemType::metricGlobal;
        coordsSpec.geographicSystem = "WGS 84";
        coordsSpec.globalMetricSystem = "UTM";
        coordsSpec.unitOfMeasurement = "m";
        return coordsSpec;
    }

    std::unique_ptr<ImageDataObject> genImage2D(
        int xDimensions = 3, int yDimensions = 4,
        const ReferencedCoordinateSystemSpecification & spec = defaultCoordsSpec())
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetDimensions(xDimensions, yDimensions, 1);
        image->AllocateScalars(VTK_FLOAT, 1);

        spec.writeToFieldData(*image->GetFieldData());

        return std::make_unique<ImageDataObject>("Image2D", *image);
    }
};


TEST_F(ImageDataObject_test, UpdateBoundsForChangedSpacing)
{
    auto imgData = vtkSmartPointer<vtkImageData>::New();
    imgData->SetExtent(0, 10, 0, 20, 0, 0);

    auto img = std::make_unique<ImageDataObject>("noname", *imgData);

    bool boundsChangedEmitted = false;

    QObject::connect(img.get(), &DataObject::boundsChanged, [&boundsChangedEmitted] ()
    {
        boundsChangedEmitted = true;
    });

    const auto initialBounds = DataBounds(img->bounds());
    ASSERT_EQ(DataBounds({ 0., 10., 0., 20., 0., 0. }), initialBounds);

    imgData->SetSpacing(0.1, 0.1, 0.1);

    ASSERT_TRUE(boundsChangedEmitted);

    const auto newBounds = DataBounds(img->bounds());
    ASSERT_EQ(DataBounds({ 0., 1., 0., 2., 0., 0. }), newBounds);
}

TEST_F(ImageDataObject_test, CorrectCoordinateSystem)
{
    auto image = genImage2D();

    ASSERT_EQ(defaultCoordsSpec(), image->coordinateSystem());
}

TEST_F(ImageDataObject_test, TransformCoords_m_to_km_metricGlobal)
{
    auto image = genImage2D();
    auto && boundsM = image->bounds();

    auto specKm = defaultCoordsSpec();
    specKm.unitOfMeasurement = "km";

    ASSERT_TRUE(image->canTransformTo(specKm));

    auto inKmDataSet = image->coordinateTransformedDataSet(specKm);
    ASSERT_TRUE(inKmDataSet);

    auto inKm = vtkImageData::SafeDownCast(inKmDataSet);
    ASSERT_TRUE(inKm);

    DataBounds boundsKm;
    inKm->GetBounds(boundsKm.data());

    ASSERT_EQ(boundsM.scaled(vtkVector3d{ 0.001, 0.001, 1.0 }), boundsKm);
}

TEST_F(ImageDataObject_test, TransformCoords_m_to_km_metricLocal)
{
    auto spec = defaultCoordsSpec();
    spec.type = CoordinateSystemType::metricLocal;
    auto image = genImage2D(3, 4, spec);
    auto && boundsM = image->bounds();

    auto specKm = spec;
    specKm.unitOfMeasurement = "km";

    ASSERT_TRUE(image->canTransformTo(specKm));

    auto inKmDataSet = image->coordinateTransformedDataSet(specKm);
    ASSERT_TRUE(inKmDataSet);

    auto inKm = vtkImageData::SafeDownCast(inKmDataSet);
    ASSERT_TRUE(inKm);

    DataBounds boundsKm;
    inKm->GetBounds(boundsKm.data());

    ASSERT_EQ(boundsM.scaled(vtkVector3d{ 0.001, 0.001, 1.0 }), boundsKm);
}
