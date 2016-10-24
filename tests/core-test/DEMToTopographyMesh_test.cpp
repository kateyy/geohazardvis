#include <gtest/gtest.h>

#include <vtkDiskSource.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/filters/DEMToTopographyMesh.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>


class DEMToTopographyMesh_test : public testing::Test
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

        return image;
    }

    static vtkSmartPointer<vtkPolyData> generateMesh()
    {
        auto disk = vtkSmartPointer<vtkDiskSource>::New();
        disk->SetInnerRadius(0);
        disk->SetOuterRadius(1);
        disk->SetCircumferentialResolution(6);
        disk->SetRadialResolution(3);
        disk->Update();

        return disk->GetOutput();
    }
};

TEST_F(DEMToTopographyMesh_test, basicRun)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);

    ASSERT_EQ(1, filter->GetExecutive()->Update());
}

TEST_F(DEMToTopographyMesh_test, MatchingParameters)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);
    filter->SetParametersToMatching();

    const auto demBounds = DataBounds(dem->GetBounds());

    // x dimension is limiting
    ASSERT_EQ(0.5 * demBounds.extractDimension(0).componentSize(), filter->GetTopographyRadius());
    ASSERT_EQ(convertTo<2>(demBounds.center()), filter->GetTopographyShiftXY());
}

TEST_F(DEMToTopographyMesh_test, ScaledTopoForMatchingParams)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);
    filter->SetParametersToMatching();

    ASSERT_EQ(1, filter->GetExecutive()->Update());
    auto shiftedTopo = filter->GetOutputTopoMeshOnDEM();
    ASSERT_TRUE(shiftedTopo);

    const auto demBounds = DataBounds(dem->GetBounds()).convertTo<2>();
    const auto shiftedTopoBounds = DataBounds(shiftedTopo->GetBounds()).convertTo<2>();

    // x dimension is limiting
    ASSERT_EQ(demBounds.extractDimension(0), shiftedTopoBounds.extractDimension(0));
    ASSERT_EQ(demBounds.center(), shiftedTopoBounds.center());
}

TEST_F(DEMToTopographyMesh_test, PreservesMeshXYRatio)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);
    filter->SetParametersToMatching();

    ASSERT_EQ(1, filter->GetExecutive()->Update());
    auto outputTopo = filter->GetOutputTopography();
    ASSERT_TRUE(outputTopo);

    const auto inputBounds = DataBounds(mesh->GetBounds()).convertTo<2>().componentSize();
    const auto outputBounds = DataBounds(outputTopo->GetBounds()).convertTo<2>().componentSize();

    const auto inputXYRatio = inputBounds[0] / inputBounds[1];
    const auto outputXYRatio = outputBounds[0] / outputBounds[1];

    ASSERT_NEAR(inputXYRatio, outputXYRatio, 0.000000048);
}

TEST_F(DEMToTopographyMesh_test, OutputElevationInInputElevationRange)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);
    filter->SetParametersToMatching();

    ASSERT_EQ(1, filter->GetExecutive()->Update());
    auto outputTopo = filter->GetOutputTopography();
    ASSERT_TRUE(outputTopo);

    const auto inputElevation = DataBounds(dem->GetBounds()).extractDimension(2);
    const auto outputElevation = DataBounds(outputTopo->GetBounds()).extractDimension(2);

    ASSERT_LE(inputElevation[0], outputElevation[0]);
    ASSERT_GE(inputElevation[1], outputElevation[1]);
}

TEST_F(DEMToTopographyMesh_test, ResetsToMatching)
{
    auto dem = generateDEM();
    auto mesh = generateMesh();

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);

    filter->SetTopographyShiftXY(vtkVector2d{ -1, -1 });
    filter->SetTopographyRadius(20);

    filter->SetParametersToMatching();

    const auto demBounds = DataBounds(dem->GetBounds());

    // x dimension is limiting
    ASSERT_EQ(0.5 * demBounds.extractDimension(0).componentSize(), filter->GetTopographyRadius());
    ASSERT_EQ(convertTo<2>(demBounds.center()), filter->GetTopographyShiftXY());
}

TEST_F(DEMToTopographyMesh_test, DEMToLocal_CorrectMeshBounds)
{
    auto dem = generateDEM();
    dem->SetOrigin(-1.5, -2.5, 0);  // make sure that tested values are != 0
    auto mesh = generateMesh();

    const auto inMeshBounds = DataBounds(mesh->GetBounds());

    auto filter = vtkSmartPointer<DEMToTopographyMesh>::New();
    filter->SetInputDEM(dem);
    filter->SetInputMeshTemplate(mesh);

    filter->SetParametersToMatching();

    filter->Update();

    const auto localDEMBounds = DataBounds(dem->GetBounds());
    const auto transformedMeshBounds = DataBounds(filter->GetOutputTopoMeshOnDEM()->GetBounds());

    const auto inMeshRatio = inMeshBounds.extractDimension(0).componentSize()
        / inMeshBounds.extractDimension(1).componentSize();
    const auto outMeshRation = transformedMeshBounds.extractDimension(0).componentSize()
        / transformedMeshBounds.extractDimension(1).componentSize();

    ASSERT_TRUE(localDEMBounds.contains(transformedMeshBounds));
    ASSERT_DOUBLE_EQ(localDEMBounds.extractDimension(0)[0], transformedMeshBounds.extractDimension(0)[0]);
    ASSERT_DOUBLE_EQ(localDEMBounds.extractDimension(0)[1], transformedMeshBounds.extractDimension(0)[1]);
    ASSERT_NEAR(inMeshRatio, outMeshRation, 0.000000016);
}
