#include <gtest/gtest.h>

#include <QApplication>

#include <vtkActor.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPropCollection.h>

#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/rendered_data/RenderedData.h>


class RenderedData_test : public testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }

};

TEST_F(RenderedData_test, RenderedPolyData_SetsMapperInfo)
{
    auto data = vtkSmartPointer<vtkPolyData>::New();

    auto dataObject = std::make_unique<PolyDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
            continue;

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}

TEST_F(RenderedData_test, ImageData_SetsMapperInfo)
{
    int argc = 0;
    char ** argv = nullptr;
    // for color mapping, GradientResourceManger is called, which create QPixmaps;
    // This requires the Qt platform integration to be setup.
    QApplication app(argc, argv);

    auto data = vtkSmartPointer<vtkImageData>::New();

    auto dataObject = std::make_unique<ImageDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
            continue;

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}

TEST_F(RenderedData_test, VectorGrid3D_SetsMapperInfo)
{
    auto data = vtkSmartPointer<vtkImageData>::New();
    data->SetDimensions(10, 10, 10);
    data->AllocateScalars(VTK_FLOAT, 3);
    data->GetPointData()->GetScalars()->SetName("nononame");

    auto dataObject = std::make_unique<VectorGrid3DDataObject>("noname", *data);

    auto rendered = dataObject->createRendered();

    DataObject * dataObjectPtr = dataObject.get();
    AbstractVisualizedData * visDataPtr = rendered.get();

    auto && viewProps = rendered->viewProps();
    for (viewProps->InitTraversal(); auto actor = vtkActor::SafeDownCast(viewProps->GetNextProp());)
    {
        auto mapper = actor->GetMapper();
        if (!mapper)
            continue;

        auto & mapperInfo = *mapper->GetInformation();

        ASSERT_EQ(DataObject::readPointer(mapperInfo), dataObjectPtr);
        ASSERT_EQ(AbstractVisualizedData::readPointer(mapperInfo), visDataPtr);
    }
}
