#include <gtest/gtest.h>

#include <memory>

#include <QApplication>

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
#include <gui/data_view/AbstractRenderView.h>
#include <gui/widgets/DEMWidget.h>

#include "app_helper.h"


class DEMWidget_test : public testing::Test
{
public:
    void SetUp() override
    {
        int argc = 1;

        env.app = std::make_unique<QApplication>(argc, main_argv);
        env.dataSetHandler = std::make_unique<DataSetHandler>();
        env.dataMapping = std::make_unique<DataMapping>(*env.dataSetHandler);
    }

    void TearDown() override
    {
        env.app.reset();
        env.dataSetHandler.reset();
        env.dataMapping.reset();
    }

    struct TestEnv
    {
        std::unique_ptr<QApplication> app;
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
        auto ptr = reinterpret_cast<float *>(demData->GetScalarPointer());
        for (size_t i = 0; i < ext.numberOfPoints(); ++i)
        {
            ptr[i] = elevationRange.min() + elevationRange.componentSize() * i / (ext.numberOfPoints() - 1.0);
        }
        demData->GetPointData()->GetScalars()->SetName("Elevations");

        return std::make_unique<ImageDataObject>(name, *demData);
    }
};

DEMWidget_test::TestEnv DEMWidget_test::env;



TEST_F(DEMWidget_test, BaseGUIRunPassToDataSetHandler)
{
    auto topo = generateTopo();
    auto dem = generateDEM();
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    const auto demBounds = DataBounds(dem->bounds());

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

    demWidget.setTransformDEMToLocalCoords(false);
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
    demWidget.setTransformDEMToLocalCoords(false);
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
    dem->imageData()->SetSpacing(0.1, 0.1, 0.1);
    // resulting DEM size: (10, 20, 0)
    env.dataSetHandler->addExternalData({ topo.get(), dem.get() });

    const auto demBounds = DataBounds(dem->bounds());

    Test_DEMWidget demWidget;

    demWidget.setDEM(dem.get());
    demWidget.setMeshTemplate(topo.get());
    demWidget.setTransformDEMToLocalCoords(false);
    demWidget.setCenterTopographyMesh(false);

    // NOTE: the minimal valid radius is defined by the DEM's spacing (0.1)
    const double topoRadius = 1.5;
    demWidget.setTopoRadius(topoRadius);
    ASSERT_EQ(topoRadius, demWidget.topoRadius());
    // DEM center is at (5, 10, 0)
    const auto topoShift = vtkVector2d(6, 8);
    demWidget.setTopoShiftXY(topoShift);
    ASSERT_EQ(topoShift, demWidget.topoShiftXY());

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
    demWidget.setTransformDEMToLocalCoords(true);
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

    auto & previewWindow = *env.dataMapping->renderViews().first();
    const auto visDataObjects = previewWindow.dataObjects();

    // simulate normal GUI behavior. The main window would normally add the render view to itself,
    // thus triggering an actual close event on shutdown. The deletion of visualization is done
    // in the close event, and it won't work correctly without showing the data view once.
    previewWindow.show();
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

    ASSERT_NO_THROW(env.dataSetHandler->deleteData({ topo1, dem1 }));

    previewRenderer.close();
}
