#include <gtest/gtest.h>

#include <array>

#include <QApplication>

#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RenderViewStrategy2D.h>

#include "app_helper.h"
#include "RenderView_test_tools.h"


class RenderView_test : public testing::Test
{
public:
    void SetUp() override
    {
        int argc = 1;

        env = std::make_unique<TestEnv>(argc, main_argv);
    }
    void TearDown() override
    {
        env.reset();
    }

    struct TestEnv
    {
        TestEnv(int & argc, char ** argv)
            : app(argc, argv)
            , dataSetHandler{}
            , dataMapping(dataSetHandler)
            , signalHelper{}
        {
        }

        QApplication app;
        DataSetHandler dataSetHandler;
        DataMapping dataMapping;
        SignalHelper signalHelper;
    };

    std::unique_ptr<TestEnv> env;

    static std::unique_ptr<PolyDataObject> genEmptyPolyData()
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();

        return std::make_unique<PolyDataObject>("PolyData", *poly);
    }

    static std::unique_ptr<PolyDataObject> genPolyData()
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();

        // texture mapping produces warnings if the data set does not contain at least 3 points
        points->InsertNextPoint(0, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(1, 1, 0);
        poly->SetPoints(points);
        std::array<vtkIdType, 3> pointIds = { 0, 1, 2 };
        poly->Allocate(static_cast<vtkIdType>(pointIds.size()));
        poly->InsertNextCell(VTK_TRIANGLE, static_cast<int>(pointIds.size()), pointIds.data());

        return std::make_unique<PolyDataObject>("PolyData", *poly);
    }
};

TEST_F(RenderView_test, AbortLoadingDeletedData)
{
    auto polyData = genEmptyPolyData();

    auto renderView = std::make_unique<RenderView>(env->dataMapping, 0);

    env->signalHelper.emitQueuedDelete(polyData.get(), renderView.get());

    QList<DataObject *> incompatible;
    renderView->showDataObjects({ polyData.get() }, incompatible);

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 0);
}

TEST_F(RenderView_test, DontMissNewDataAtSameLocation)
{
    // texture mapping produces warnings if the data set does not contain at least 3 points
    auto polyData = genPolyData();
    vtkSmartPointer<vtkPolyData> polyDataSet = &polyData->polyDataSet();

    auto renderView = std::make_unique<RenderView>(env->dataMapping, 0);

    QList<DataObject *> incompatible;
    renderView->showDataObjects({ polyData.get() }, incompatible);
    renderView->prepareDeleteData({ polyData.get() });

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 0);

    // create new object at same location, emulating a case that can randomly happen in practice
    polyData->~PolyDataObject();
    new (polyData.get()) PolyDataObject("PolyData", *polyDataSet);

    renderView->showDataObjects({ polyData.get() }, incompatible);

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 1);
}

