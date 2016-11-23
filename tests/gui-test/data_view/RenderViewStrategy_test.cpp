#include <gtest/gtest.h>

#include <vtkImageData.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/DataSetHandler.h>

#include <gui/DataMapping.h>
#include <gui/MainWindow.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>


class RenderViewStrategy_test : public ::testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
};

TEST_F(RenderViewStrategy_test, CreateCorrectNumberOfPlots)
{
    auto mainWindow = std::make_unique<MainWindow>();

    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, 1, 0, 1, 0, 0);
    image->AllocateScalars(VTK_FLOAT, 1);
    auto imageData = std::make_unique<ImageDataObject>("image", *image);

    DataSetHandler dataSetHandler;
    DataMapping dataMapping(dataSetHandler);
    auto renderView = std::make_unique<ResidualVerificationView>(dataMapping, 0);

    renderView->setObservationData(imageData.get());
    renderView->setModelData(imageData.get());
    renderView->waitForResidualUpdate();

    auto & strategy = dynamic_cast<RendererImplementationResidual &>(renderView->implementation()).strategy2D();

    // here is the code that is actually been tested here
    strategy.startProfilePlot();

    strategy.acceptProfilePlot();

    // one object in two sub-views -> only one plot required; plus the residual; the source image is not added to the handler
    ASSERT_EQ(dataSetHandler.dataSets().size(), 2);
}