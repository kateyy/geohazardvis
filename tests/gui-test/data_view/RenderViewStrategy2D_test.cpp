/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PointCloudDataObject.h>
#include <core/DataSetHandler.h>

#include <gui/DataMapping.h>
#include <gui/MainWindow.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/data_view/RendererImplementationResidual.h>
#include <gui/data_view/RenderViewStrategy2D.h>


class TestRendererImplementation3D : public RendererImplementation3D
{
public:
    using RendererImplementation3D::RendererImplementation3D;
    using RendererImplementation3D::strategy;
};

class RenderViewStrategy2D_test : public ::testing::Test
{
public:
    void SetUp() override
    {
    }
    void TearDown() override
    {
    }
};

TEST_F(RenderViewStrategy2D_test, CreateCorrectNumberOfPlots)
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

TEST_F(RenderViewStrategy2D_test, CancelPlotWhenHidingSource)
{
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, 1, 0, 1, 0, 0);
    image->AllocateScalars(VTK_FLOAT, 1);
    auto imageData = std::make_unique<ImageDataObject>("image", *image);

    DataSetHandler dataSetHandler;
    DataMapping dataMapping(dataSetHandler);
    auto renderView = dataMapping.openInRenderView({ imageData.get() });

    auto impl = dynamic_cast<RendererImplementation3D *>(&renderView->implementation());
    ASSERT_TRUE(impl);
    auto & strategy = static_cast<TestRendererImplementation3D *>(impl)->strategy();
    auto strategy2D = dynamic_cast<RenderViewStrategy2D *>(&strategy);
    ASSERT_TRUE(strategy2D);

    strategy2D->startProfilePlot();
    ASSERT_TRUE(strategy2D->plotPreviewRenderer());
    auto && plots = strategy2D->plotPreviewRenderer()->dataObjects();
    ASSERT_EQ(1, plots.size());

    renderView->hideDataObjects({ imageData.get() });

    ASSERT_FALSE(strategy2D->plotPreviewRenderer());
}

TEST_F(RenderViewStrategy2D_test, CancelPlotWhenHidingSourceWithOtherDataPlotted)
{
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, 1, 0, 1, 0, 0);
    image->AllocateScalars(VTK_FLOAT, 1);
    auto imageData = std::make_unique<ImageDataObject>("image", *image);
    auto otherData = std::make_unique<ImageDataObject>("other image", *image);

    DataSetHandler dataSetHandler;
    DataMapping dataMapping(dataSetHandler);
    auto renderView = dataMapping.openInRenderView({ imageData.get(), otherData.get() });

    auto impl = dynamic_cast<RendererImplementation3D *>(&renderView->implementation());
    ASSERT_TRUE(impl);
    auto & strategy = static_cast<TestRendererImplementation3D *>(impl)->strategy();
    auto strategy2D = dynamic_cast<RenderViewStrategy2D *>(&strategy);
    ASSERT_TRUE(strategy2D);

    strategy2D->startProfilePlot();
    ASSERT_TRUE(strategy2D->plotPreviewRenderer());
    ASSERT_EQ(2, strategy2D->plotPreviewRenderer()->dataObjects().size());

    renderView->hideDataObjects({ imageData.get() });

    ASSERT_TRUE(strategy2D->plotPreviewRenderer());
    ASSERT_EQ(1, strategy2D->plotPreviewRenderer()->dataObjects().size());
}

TEST_F(RenderViewStrategy2D_test, CancelPlotWhenHidingSourceWithOtherDataShown)
{
    auto image = vtkSmartPointer<vtkImageData>::New();
    image->SetExtent(0, 1, 0, 1, 0, 0);
    image->AllocateScalars(VTK_FLOAT, 1);
    auto imageData = std::make_unique<ImageDataObject>("image", *image);

    auto noPlotData = vtkSmartPointer<vtkPolyData>::New();
    auto noPlotPoints = vtkSmartPointer<vtkPoints>::New();
    noPlotPoints->SetNumberOfPoints(3);
    noPlotPoints->SetPoint(0, 0, 0, 0);
    noPlotPoints->SetPoint(1, 1, 2, 0);
    noPlotPoints->SetPoint(2, 3, 4, 0);
    noPlotData->SetPoints(noPlotPoints);
    auto noPlotVerts = vtkSmartPointer<vtkCellArray>::New();
    noPlotVerts->InsertNextCell(3, std::vector<vtkIdType>({ 0, 1, 2 }).data());
    noPlotData->SetVerts(noPlotVerts);
    auto otherData = std::make_unique<PointCloudDataObject>("noPlotData", *noPlotData);

    DataSetHandler dataSetHandler;
    DataMapping dataMapping(dataSetHandler);
    auto renderView = dataMapping.openInRenderView({ imageData.get(), otherData.get() });

    auto impl = dynamic_cast<RendererImplementation3D *>(&renderView->implementation());
    ASSERT_TRUE(impl);
    auto & strategy = static_cast<TestRendererImplementation3D *>(impl)->strategy();
    auto strategy2D = dynamic_cast<RenderViewStrategy2D *>(&strategy);
    ASSERT_TRUE(strategy2D);

    strategy2D->startProfilePlot();
    ASSERT_TRUE(strategy2D->plotPreviewRenderer());
    ASSERT_EQ(1, strategy2D->plotPreviewRenderer()->dataObjects().size());

    renderView->hideDataObjects({ imageData.get() });

    ASSERT_FALSE(strategy2D->plotPreviewRenderer());
}
