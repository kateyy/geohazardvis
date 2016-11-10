#include <gtest/gtest.h>

#include <array>
#include <cassert>
#include <memory>

#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkVector.h>

#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/vtkvectorhelper.h>

#include <gui/rendering_interaction/Picker.h>


class Picker_test : public testing::Test
{
public:
    void SetUp() override
    {
        renWin = vtkSmartPointer<vtkRenderWindow>::New();
        renWin->OffScreenRenderingOn();
        ren = vtkSmartPointer<vtkRenderer>::New();
        renWin->AddRenderer(ren);
        renWin->SetSize(renWinSize.GetX(), renWinSize.GetY());
        // required on Intel/Linux to prevent seg faults in later render calls
        renWin->Start();
    }

    void setupAddPolyData(const QString & name = "Poly")
    {
        dataObject = genPolyData(name);
        renderedData = dataObject->createRendered();

        addViewProps();
    }

    void lookAtPolyData()
    {
        auto & cam = *ren->GetActiveCamera();
        cam.SetPosition(0.0, 1.0, 0.0);
        cam.SetFocalPoint(0.0, 0.0, 0.0);
        cam.SetViewUp(0.0, 0.0, 1.0);
        ren->ResetCamera();
        ren->Render();
    }

    vtkVector2i polyDataCell1PickPos()
    {
        return convertTo<int>((convertTo<double>(renWinSize) * 0.6));
    }

    void setupAddImageData(const QString & name = "Image")
    {
        dataObject = genImageData(name);
        renderedData = dataObject->createRendered();
        renderedData->colorMapping();   // initialize

        addViewProps();
    }

    void lookAtImageData()
    {
        auto & cam = *ren->GetActiveCamera();

        cam.ParallelProjectionOn();
        cam.SetViewUp(0, 1, 0);
        cam.SetFocalPoint(1, 1.5, 0);
        cam.SetPosition(1, 1.5, 10.25);
        ren->ResetCamera();
        cam.SetParallelScale(1.5);
        ren->ResetCameraClippingRange();
        ren->Render();
    }

    vtkVector2i imageDataPoint8PickPos()
    {
        return convertTo<int>((convertTo<double>(renWinSize) *
            vtkVector2d(0.99, 0.66)));
    }

    void addViewProps()
    {
        auto viewProps = renderedData->viewProps();
        viewProps->InitTraversal();
        while (auto prop = viewProps->GetNextProp())
        {
            ren->AddViewProp(prop);
        }
    }

    const vtkVector2i renWinSize = vtkVector2i(200, 300);
    vtkSmartPointer<vtkRenderer> ren;
    vtkSmartPointer<vtkRenderWindow> renWin;

    std::unique_ptr<DataObject> dataObject;
    std::unique_ptr<RenderedData> renderedData;


    static std::unique_ptr<PolyDataObject> genPolyData(const QString & name = "Poly")
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();

        points->InsertNextPoint(-1, 0, -1);
        points->InsertNextPoint(1, 0, -1);
        points->InsertNextPoint(1, 0, 1);
        points->InsertNextPoint(-1, 0, 1);
        poly->SetPoints(points);
        std::array<vtkIdType, 6> pointIds = {
            0, 1, 2,
            1, 2, 3
        };
        poly->Allocate(static_cast<vtkIdType>(pointIds.size()));
        poly->InsertNextCell(VTK_TRIANGLE, 3, pointIds.data());
        poly->InsertNextCell(VTK_TRIANGLE, 3, pointIds.data() + 3);

        auto scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetName(name.toUtf8().data());
        scalars->SetNumberOfTuples(poly->GetNumberOfCells());
        for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
        {
            scalars->SetValue(i, static_cast<float>(i));
        }
        poly->GetCellData()->SetScalars(scalars);

        return std::make_unique<PolyDataObject>(name, *poly);
    }

    static std::unique_ptr<ImageDataObject> genImageData(const QString & name = "Image")
    {
        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetExtent(0, 2, 0, 3, 0, 0);

        auto scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfTuples(image->GetNumberOfPoints());
        scalars->SetName(name.toUtf8().data());

        for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
        {
            scalars->SetValue(i, static_cast<float>(i));
        }

        image->GetPointData()->SetScalars(scalars);

        return std::make_unique<ImageDataObject>(name, *image);
    }
};


TEST_F(Picker_test, PickPolyData)
{
    setupAddPolyData();
    lookAtPolyData();

    auto picker = Picker();
    picker.pick(polyDataCell1PickPos(), *ren);

    ASSERT_EQ(
        VisualizationSelection(renderedData.get(), 0, IndexType::cells, 1),
        picker.pickedObjectInfo()
    );
}

TEST_F(Picker_test, PickImageData)
{
    setupAddImageData();

    lookAtImageData();

    auto picker = Picker();
    picker.pick(imageDataPoint8PickPos(), *ren);

    ASSERT_EQ(
        VisualizationSelection(renderedData.get(), 0, IndexType::points, 8),
        picker.pickedObjectInfo()
    );
}

TEST_F(Picker_test, PickPolyData_noColorMappingNoScalars)
{
    setupAddPolyData();
    lookAtPolyData();

    renderedData->colorMapping().setEnabled(false);

    auto picker = Picker();
    picker.pick(polyDataCell1PickPos(), *ren);

    ASSERT_EQ(nullptr, picker.pickedScalarArray());
}

TEST_F(Picker_test, PickImageData_noColorMappingNoScalars)
{
    setupAddImageData();
    lookAtImageData();

    renderedData->colorMapping().setEnabled(false);

    auto picker = Picker();
    picker.pick(imageDataPoint8PickPos(), *ren);

    ASSERT_EQ(nullptr, picker.pickedScalarArray());
}

TEST_F(Picker_test, PickPolyData_pickMappedScalarArray)
{
    const QString scalarsName = "Poly Data";
    setupAddPolyData(scalarsName);
    lookAtPolyData();

    auto scalars = dataObject->dataSet()->GetCellData()->GetScalars();
    assert(scalars);

    renderedData->colorMapping().setCurrentScalarsByName(scalarsName, true);

    auto picker = Picker();
    picker.pick(polyDataCell1PickPos(), *ren);

    ASSERT_EQ(scalars, picker.pickedScalarArray());
}

TEST_F(Picker_test, PickImageData_pickMappedScalarArray)
{
    const QString scalarsName = "An Image";
    setupAddImageData(scalarsName);
    lookAtImageData();

    auto scalars = dataObject->dataSet()->GetPointData()->GetScalars();
    assert(scalars);

    // this should be selected by default
    //renderedData->colorMapping().setCurrentScalarsByName(scalarsName, true);

    auto picker = Picker();
    picker.pick(imageDataPoint8PickPos(), *ren);

    ASSERT_EQ(scalars, picker.pickedScalarArray());
}
