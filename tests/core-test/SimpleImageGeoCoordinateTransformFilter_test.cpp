#include <gtest/gtest.h>

#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/CoordinateSystems.h>
#include <core/filters/SimpleImageGeoCoordinateTransformFilter.h>
#include <core/utility/DataExtent.h>


class SimpleDEMGeoCoordToLocalFilter_test : public testing::Test
{
public:
    static vtkSmartPointer<vtkImageData> generateDEM()
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetExtent(0, 10, 0, 20, 0, 0);

        auto elevations = vtkSmartPointer<vtkFloatArray>::New();
        elevations->SetNumberOfValues(image->GetNumberOfPoints());
        elevations->SetName("elevations");

        for (int i = 0; i < elevations->GetNumberOfValues(); ++i)
        {
            elevations->SetValue(i, static_cast<float>(i / (elevations->GetNumberOfValues() - 1)));
        }

        image->GetPointData()->SetScalars(elevations);
        const auto centerXY = DataBounds(image->GetBounds()).center();

        ReferencedCoordinateSystemSpecification(
            CoordinateSystemType::geographic,
            "some geo system",
            "some metric system",
            { centerXY[1], centerXY[0] },
            { 0.5, 0.5 }
        ).writeToFieldData(*image->GetFieldData());

        return image;
    }

};


TEST_F(SimpleDEMGeoCoordToLocalFilter_test, OutputMeshIsLocal)
{
    auto filter = vtkSmartPointer<SimpleImageGeoCoordinateTransformFilter>::New();
    auto dem = generateDEM();

    filter->SetInputData(dem);
    ASSERT_TRUE(filter->GetExecutive()->Update());
    auto localDem = filter->GetOutput();

    DataBounds bounds;
    localDem->GetBounds(bounds.data());
    const auto bounds2D = bounds.convertTo<2>();

    ASSERT_FALSE(bounds.isEmpty());
    ASSERT_EQ(vtkVector2d(0, 0), bounds2D.center());
}
