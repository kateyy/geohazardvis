#include <gtest/gtest.h>

#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <core/filters/SimpleDEMGeoCoordToLocalFilter.h>
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
            elevations->SetValue(i, i / (elevations->GetNumberOfValues() - 1));
        }

        image->GetPointData()->SetScalars(elevations);

        return image;
    }

};


TEST_F(SimpleDEMGeoCoordToLocalFilter_test, OutputMeshIsLocal)
{
    auto filter = vtkSmartPointer<SimpleDEMGeoCoordToLocalFilter>::New();
    auto dem = generateDEM();

    filter->SetInputData(dem);
    filter->Update();
    auto localDem = filter->GetOutput();

    DataBounds bounds;
    localDem->GetBounds(bounds.data());
    const auto bounds2D = bounds.convertTo<2>();

    ASSERT_FALSE(bounds.isEmpty());
    ASSERT_EQ(vtkVector2d(0, 0), bounds2D.center());
}
