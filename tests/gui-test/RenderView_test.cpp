#include <gtest/gtest.h>

#include <QApplication>

#include <vtkPolyData.h>

#include <core/data_objects/PolyDataObject.h>

#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RenderViewStrategy2D.h>

#include "app_helper.h"
#include "RenderView_test_tools.h"


class RenderView_test: public testing::Test
{
public:
    void SetUp() override
    {
        int argc = 1;

        app = std::make_unique<QApplication>(argc, main_argv);
    }
    void TearDown() override
    {
    }

    std::unique_ptr<QApplication> app;
    SignalHelper signalHelper;
};

TEST_F(RenderView_test, AbortLoadingDeletedData)
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto polyData = std::make_unique<PolyDataObject>("PolyData", *poly);

    auto renderView = std::make_unique<RenderView>(0);

    signalHelper.emitQueuedDelete(polyData.get(), renderView.get());

    QList<DataObject *> incompatible;
    renderView->showDataObjects({ polyData.get() }, incompatible);

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 0);
}

TEST_F(RenderView_test, DontMissNewDataAtSameLocation)
{
    auto poly = vtkSmartPointer<vtkPolyData>::New();
    auto polyData = std::make_unique<PolyDataObject>("PolyData", *poly);

    auto renderView = std::make_unique<RenderView>(0);

    QList<DataObject *> incompatible;
    renderView->showDataObjects({ polyData.get() }, incompatible);
    renderView->prepareDeleteData({ polyData.get() });

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 0);

    // create new object at same location, emulating a case that can randomly happen in practice
    polyData->~PolyDataObject();
    new (polyData.get()) PolyDataObject("PolyData", *poly);

    renderView->showDataObjects({ polyData.get() }, incompatible);

    ASSERT_EQ(incompatible.size(), 0);
    ASSERT_EQ(renderView->dataObjects().size(), 1);
}

