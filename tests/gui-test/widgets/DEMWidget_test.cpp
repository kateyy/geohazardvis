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

#include <memory>

#include <QApplication>

#include <vtkDataArray.h>
#include <vtkDiskSource.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/io/MatricesToVtk.h>
#include <core/utility/DataExtent.h>
#include <core/utility/vtkvectorhelper.h>
#include <gui/DataMapping.h>
#include <gui/data_handling/DEMWidget.h>
#include <gui/data_view/AbstractRenderView.h>


class DEMWidget_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        env.dataSetHandler = std::make_unique<DataSetHandler>();
        env.dataMapping = std::make_unique<DataMapping>(*env.dataSetHandler);
    }

    void TearDown() override
    {
        env.dataSetHandler.reset();
        env.dataMapping.reset();
    }

    struct TestEnv
    {
        std::unique_ptr<DataSetHandler> dataSetHandler;
        std::unique_ptr<DataMapping> dataMapping;
    };

    class Test_DEMWidget : public DEMWidget
    {
    public:
        Test_DEMWidget()
            : DEMWidget(*env.dataMapping)
        {
        }
    };

    static TestEnv env;


    static std::unique_ptr<PolyDataObject> generateTopo(const QString & name = "Topo")
    {
        const double r = 10.f;
        const double top = 0, right = 1, bottom = 2, left = 3, center = 4;

        io::InputVector points = {
            /* id */ {top, right, bottom, left, center},
            /* x  */ {r, 0, -r, 0, 0},
            /* y  */ {0, r, 0, -r, 0},
            /* z  */ {0, 0, 0, 0, 0}
        };
        io::InputVector triangleIds = {
            {center, center, center, center},
            {top, right, bottom, left},
            {right, bottom, left, top}
        };
        auto poly = MatricesToVtk::parseIndexedTriangles(
            points, 0, 1,
            triangleIds, 0);

        return std::make_unique<PolyDataObject>(name, *poly);
    }

    static std::unique_ptr<ImageDataObject> generateDEM(const QString & name)
    {
        return generateDEM(ImageExtent({ 0, 10, 0, 20, 0, 0 }),
            ValueRange<double>({ -3, 3 }),
            name);
    }

    static std::unique_ptr<ImageDataObject> generateDEM(
        const ImageExtent & extent = ImageExtent({0, 10, 0, 20, 0, 0}),
        const ValueRange<double> & elevationRange = ValueRange<double>({ -3, 3 }),
        const QString & name = "DEM")
    {
        auto ext = extent;
        auto demData = vtkSmartPointer<vtkImageData>::New();
        demData->SetExtent(ext.data());
        demData->AllocateScalars(VTK_FLOAT, 1);
        auto scalars = demData->GetPointData()->GetScalars();
        for (size_t i = 0; i < ext.numberOfPoints(); ++i)
        {
            const auto val = elevationRange.min() + elevationRange.componentSize() * 
                static_cast<double>(i) / (static_cast<double>((ext.numberOfPoints()) - 1.0));
            assert(static_cast<vtkIdType>(i) < scalars->GetNumberOfTuples());
            scalars->SetComponent(static_cast<vtkIdType>(i), 0, val);
        }
        demData->GetPointData()->GetScalars()->SetName("Elevations");

        auto img = std::make_unique<ImageDataObject>(name, *demData);
        img->specifyCoordinateSystem(ReferencedCoordinateSystemSpecification(
            CoordinateSystemType::geographic,
            "a geo system",
            "a metric system",
            "",
            { img->bounds().center()[1], img->bounds().center()[0] }
        ));

        return img;
    }
};

DEMWidget_test::TestEnv DEMWidget_test::env;



TEST_F(DEMWidget_test, BaseGUIRunPassToDataSetHandler)
{
    auto topo = generateTopo();
    auto dem = generateDEM();
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    Test_DEMWidget demWidget;

    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());

    ASSERT_TRUE(demWidget.save());

    ASSERT_EQ(3, env.dataSetHandler->dataSets().count());
    const auto dataSets = env.dataSetHandler->dataSets();
    const auto it = std::find_if(dataSets.begin(), dataSets.end(),
        [&topo, &dem] (DataObject * dataObject)
    {
        return dataObject != topo.get() && dataObject != dem.get();
    });
    ASSERT_NE(it, dataSets.end());
    ASSERT_TRUE(dynamic_cast<PolyDataObject *>(*it));
}

TEST_F(DEMWidget_test, DefaultConfigIsMatching)
{
    auto topo = generateTopo();
    auto dem = generateDEM();
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    const auto demBounds = DataBounds(dem->bounds());

    Test_DEMWidget demWidget;

    demWidget.setCenterTopographyMesh(false);
    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());

    auto newMesh = demWidget.saveRelease();

    ASSERT_TRUE(newMesh);

    const auto bounds = DataBounds(newMesh->bounds());
    ASSERT_EQ(demBounds.center(), bounds.center());
    ASSERT_EQ(demBounds.componentSize()[0], bounds.componentSize()[0]);
    ASSERT_EQ(demBounds.componentSize()[0], bounds.componentSize()[1]);    // height (y) should be limited to DEM's height
}

TEST_F(DEMWidget_test, DefaultConfigCreatesElevations)
{
    auto topo = generateTopo();
    auto dem = generateDEM();
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    ValueRange<double> demElevationRange;
    dem->dataSet()->GetPointData()->GetScalars()->GetRange(demElevationRange.data());

    Test_DEMWidget demWidget;
    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());
    demWidget.setCenterTopographyMesh(false);

    auto newMesh = demWidget.saveRelease();

    auto newElevationRange = DataBounds(newMesh->bounds()).extractDimension(2);
    ASSERT_TRUE(demElevationRange.contains(newElevationRange));
    ASSERT_GT(newElevationRange.componentSize(), 0.0);
}

TEST_F(DEMWidget_test, ResetToDefaults)
{
    auto topo = generateTopo();
    auto dem = generateDEM(ImageExtent({ 0, 100, 0, 200, 0, 0 }));
    dem->imageData().SetSpacing(0.1, 0.1, 0.1);
    // resulting DEM size: (10, 20, 0)
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    const auto demBounds = DataBounds(dem->bounds());

    Test_DEMWidget demWidget;

    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());
    demWidget.setCenterTopographyMesh(false);

    // NOTE: the minimal valid radius is defined by the DEM's spacing (0.1)
    const double topoRadius = 1.5;
    demWidget.setTopoRadius(topoRadius);
    ASSERT_EQ(topoRadius, demWidget.topoRadius());
    // DEM center is at (5, 10, 0)
    const auto topoShift = vtkVector2d(6, 8);
    demWidget.setTopographyCenterXY(topoShift);
    ASSERT_EQ(topoShift, demWidget.topographyCenterXY());

    demWidget.resetParametersForCurrentInputs();

    const auto newMesh = demWidget.saveRelease();

    const auto bounds = DataBounds(newMesh->bounds());
    ASSERT_EQ(demBounds.center(), bounds.center());
    ASSERT_EQ(demBounds.componentSize()[0], bounds.componentSize()[0]);
    ASSERT_EQ(demBounds.componentSize()[0], bounds.componentSize()[1]);    // height (y) should be limited to DEM's height
}

TEST_F(DEMWidget_test, DEMTransformedToLocal)
{
    auto topo = generateTopo();
    auto dem = generateDEM();

    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    ValueRange<double> demElevationRange;
    dem->dataSet()->GetPointData()->GetScalars()->GetRange(demElevationRange.data());

    Test_DEMWidget demWidget;

    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());
    demWidget.setTargetCoordinateSystem(CoordinateSystemType::metricLocal);
    demWidget.setCenterTopographyMesh(false);

    demWidget.resetParametersForCurrentInputs();

    const auto newMesh = demWidget.saveRelease();

    const auto newElevationRange = DataBounds(newMesh->bounds()).extractDimension(2);

    ASSERT_TRUE(demElevationRange.contains(newElevationRange));
    ASSERT_GT(newElevationRange.componentSize(), 0.0);
    ASSERT_TRUE(newElevationRange.contains(0.0));
}

TEST_F(DEMWidget_test, ShowPreview)
{
    auto topo = generateTopo("topo1");
    auto dem = generateDEM("dem1");

    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    Test_DEMWidget demWidget;
    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());
    ASSERT_NO_THROW(demWidget.showPreview());

    ASSERT_EQ(1, env.dataMapping->renderViews().size());
}

TEST_F(DEMWidget_test, DeleteCurrentDemAndTopo)
{
    auto dem1 = static_cast<ImageDataObject *>(env.dataSetHandler->takeData(generateDEM()));
    auto topo1 = static_cast<PolyDataObject *>(env.dataSetHandler->takeData(generateTopo()));

    auto topo2 = generateTopo("topo2");
    auto dem2 = generateDEM("dem2");

    env.dataSetHandler->addExternalData({ topo2.get(), dem2.get() });

    Test_DEMWidget demWidget;
    demWidget.setDEM(dem1);
    demWidget.setMeshTemplate(topo1);
    demWidget.showPreview();
    auto & previewRenderer = *env.dataMapping->renderViews().first();
    previewRenderer.show();

    ASSERT_NO_THROW(env.dataSetHandler->deleteData({ dem1, topo1 }));

    previewRenderer.close();
    qApp->processEvents();
}
