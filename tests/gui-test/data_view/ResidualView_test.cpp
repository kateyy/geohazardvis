#include <gtest/gtest.h>

#include <array>

#include <QCoreApplication>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/AbstractVisualizedData.h>
#include <core/CoordinateSystems.h>
#include <core/DataSetHandler.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/DataExtent.h>

#include <gui/DataMapping.h>
#include <gui/MainWindow.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/RendererImplementationResidual.h>

#include "RenderView_test_tools.h"


class ResidualView_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        env = std::make_unique<TestEnv>();
        view = nullptr;
    }
    void TearDown() override
    {
        if (view)
        {
            view->waitForResidualUpdate();
            view->close();
            qApp->processEvents();
        }

        env.reset();
    }

    struct TestEnv
    {
        TestEnv()
            : dataSetHandler{}
            , dataMapping(dataSetHandler)
            , signalHelper{}
        {
        }

        DataSetHandler dataSetHandler;
        DataMapping dataMapping;
        SignalHelper signalHelper;
    };

    ResidualVerificationView * view;

    std::unique_ptr<TestEnv> env;

    static std::unique_ptr<PolyDataObject> genPolyData(const QString & name, int numComponents = 1)
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

        auto scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetName(name.toUtf8().data());
        scalars->SetNumberOfComponents(numComponents);
        scalars->SetNumberOfTuples(poly->GetNumberOfCells());
        const int maxComponent = numComponents - 1;
        for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
        {
            scalars->SetTypedComponent(i, maxComponent, static_cast<float>(i));
        }
        for (int c = 0; c < maxComponent; ++c)
        {
            scalars->FillTypedComponent(c, 0.0f);
        }
        poly->GetCellData()->SetScalars(scalars);

        return std::make_unique<PolyDataObject>(name, *poly);
    }
};

TEST_F(ResidualView_test, ComputeResidual)
{
    auto ownedObservation = genPolyData("observation");
    auto ownedModel = genPolyData("displacement vectors");
    auto observation = ownedObservation.get();
    auto model = ownedModel.get();
    env->dataSetHandler.takeData(std::move(ownedObservation));
    env->dataSetHandler.takeData(std::move(ownedModel));

    auto observationValues = observation->dataSet()->GetCellData()->GetScalars();
    auto modelValues = model->dataSet()->GetCellData()->GetScalars();
    const auto numValues = observationValues->GetNumberOfTuples();
    assert(numValues == modelValues->GetNumberOfValues()
        && observationValues->GetNumberOfComponents() == 1);
    for (vtkIdType i = 0; i < numValues; ++i)
    {
        observationValues->SetComponent(i, 0, static_cast<double>(i));
        modelValues->SetComponent(i, 0, static_cast<double>(numValues - 1 - i));
    }

    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;
    residualView->setResidualGeometrySource(ResidualVerificationView::InputData::model);
    residualView->setObservationData(observation);
    residualView->setModelData(model);
    residualView->waitForResidualUpdate();

    ASSERT_EQ(observation, residualView->observationData());
    ASSERT_EQ(model, residualView->modelData());
    auto residual = residualView->residualData();
    ASSERT_TRUE(residual);
    auto residualPolyDataObject = dynamic_cast<PolyDataObject *>(residual);
    ASSERT_TRUE(residualPolyDataObject);
    auto & sourcePoly = model->polyDataSet();
    auto & residualPoly = residualPolyDataObject->polyDataSet();
    ASSERT_EQ(sourcePoly.GetNumberOfPoints(), residualPoly.GetNumberOfPoints());
    ASSERT_EQ(sourcePoly.GetNumberOfCells(), residualPoly.GetNumberOfCells());
    ASSERT_EQ(sourcePoly.GetNumberOfPolys(), residualPoly.GetNumberOfPolys());

    auto residualValues = residualPoly.GetCellData()->GetScalars();
    ASSERT_TRUE(residualValues);
    ASSERT_EQ(1, residualValues->GetNumberOfComponents());
    ASSERT_EQ(numValues, residualValues->GetNumberOfTuples());
    for (vtkIdType i = 0; i < numValues; ++i)
    {
        const auto r = static_cast<double>(i - (numValues - 1 - i));
        ASSERT_EQ(r, residualValues->GetComponent(i, 0));
    }
}

TEST_F(ResidualView_test, AbortLoadingDeletedData)
{
    auto observation = genPolyData("observation");
    auto model = genPolyData("displacement vectors");

    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;

    env->signalHelper.emitRepeatedQueuedDelete(observation.get(), residualView);
    residualView->setObservationData(observation.get());

    residualView->setModelData(model.get());
    env->signalHelper.emitRepeatedQueuedDelete(model.get(), residualView);

    residualView->waitForResidualUpdate();

    ASSERT_EQ(nullptr, residualView->observationData());
    ASSERT_EQ(nullptr, residualView->modelData());
    ASSERT_EQ(residualView->dataObjects().size(), 0);
}

TEST_F(ResidualView_test, UnselectDeletedData)
{
    auto observation = genPolyData("observation");
    auto model = genPolyData("model");

    env->dataSetHandler.addExternalData({ observation.get(), model.get() });
    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;

    residualView->setObservationData(observation.get());
    residualView->setModelData(model.get());

    env->dataMapping.removeDataObjects({ observation.get(), model.get() });
    env->dataSetHandler.removeExternalData({ observation.get(), model.get() });
    observation.reset();
    model.reset();

    auto selection = residualView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);
}

TEST_F(ResidualView_test, UnselectHiddendData)
{
    auto observation = genPolyData("observation");
    auto model = genPolyData("model");

    env->dataSetHandler.addExternalData({ observation.get(), model.get() });
    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;

    residualView->setObservationData(observation.get());
    residualView->setModelData(model.get());

    residualView->hideDataObjects({ observation.get(), model.get() });

    auto selection = residualView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);

    env->dataMapping.removeDataObjects({ observation.get(), model.get() });
    env->dataSetHandler.removeExternalData({ observation.get(), model.get() });

    residualView->waitForResidualUpdate();
}

TEST_F(ResidualView_test, SwitchInputData)
{
    auto obs1 = genPolyData("observation1");
    auto obs2 = genPolyData("observation2");

    env->dataSetHandler.addExternalData({ obs1.get(), obs2.get() });
    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;

    residualView->setObservationData(obs1.get());
    
    ASSERT_NO_THROW(residualView->setObservationData(obs2.get()));
    ASSERT_EQ(obs2.get(), residualView->observationData());

    env->dataMapping.removeDataObjects({ obs1.get(), obs2.get() });
    env->dataSetHandler.removeExternalData({ obs1.get(), obs2.get() });

    residualView->waitForResidualUpdate();
}

TEST_F(ResidualView_test, AddRemoveResidualToDataSetHandler)
{
    auto ownedObservation = genPolyData("observation");
    auto ownedModel = genPolyData("displacement vectors");
    auto observation = ownedObservation.get();
    auto model = ownedModel.get();
    env->dataSetHandler.takeData(std::move(ownedObservation));
    env->dataSetHandler.takeData(std::move(ownedModel));

    {
        auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();

        ASSERT_EQ(2, env->dataSetHandler.dataSets().size());

        residualView->setObservationData(observation);
        residualView->setModelData(model);
        residualView->waitForResidualUpdate();

        ASSERT_EQ(3, env->dataSetHandler.dataSets().size());
        ASSERT_TRUE(env->dataSetHandler.dataSets().contains(residualView->residualData()));

        residualView->close();
        qApp->processEvents();
    }

    ASSERT_EQ(2, env->dataSetHandler.dataSets().size());
}

TEST_F(ResidualView_test, CloseAppWhileResidualIsShown)
{
    env.reset();

    auto ownedObservation = genPolyData("observation");
    auto observationPtr = ownedObservation.get();
    auto ownedModel = genPolyData("displacement vectors");
    auto modelPtr = ownedModel.get();

    {
        MainWindow mainWindow;

        auto residualView = mainWindow.dataMapping().createRenderView<ResidualVerificationView>();
        residualView->setObservationData(observationPtr);
        residualView->setModelData(modelPtr);
        residualView->waitForResidualUpdate();
    }
}

TEST_F(ResidualView_test, DeferVisualizationUpdateOnLockedData)
{
    env.reset();

    auto ownedObservation = genPolyData("observation");
    auto observationPtr = ownedObservation.get();
    auto ownedModel = genPolyData("displacement vectors");
    auto modelPtr = ownedModel.get();

    {
        MainWindow mainWindow;

        auto residualView = mainWindow.dataMapping().createRenderView<ResidualVerificationView>();
        {
            const auto deferral = ownedModel->scopedEventDeferral();
            residualView->setObservationData(observationPtr);
            residualView->setModelData(modelPtr);
            residualView->waitForResidualUpdate();

            auto modelVis = residualView->visualizationFor(modelPtr);
            ASSERT_TRUE(modelVis);
            ASSERT_FALSE(modelVis->colorMapping().isEnabled());
        }
        qApp->processEvents();

        auto modelVis = residualView->visualizationFor(modelPtr);
        ASSERT_TRUE(modelVis);
        ASSERT_TRUE(modelVis->colorMapping().isEnabled());
        ASSERT_STREQ("displacement vectors", modelVis->colorMapping().currentScalarsName().toUtf8().data());
    }
}

TEST_F(ResidualView_test, ProjectToLoS_TransformedCoordinateSystem)
{
    const auto coordsSpec = ReferencedCoordinateSystemSpecification(
        CoordinateSystemType::geographic,
        "WGS 84",
        "UTM",
        {},
        vtkVector2d(-54.483333, -37.083333),
        vtkVector2d(0, 0)
    );

    auto ownedObservation = genPolyData("lineOfSightObservation");
    auto ownedModel = genPolyData("dispVectorModel", 3);
    auto observation = ownedObservation.get();
    auto model = ownedModel.get();
    env->dataSetHandler.takeData(std::move(ownedObservation));
    env->dataSetHandler.takeData(std::move(ownedModel));

    observation->polyDataSet().GetPoints()->SetPoint(0, -37.083333, -54.483333, 0);
    observation->polyDataSet().GetPoints()->SetPoint(1, -37.083333, -54.493333, 0);
    observation->polyDataSet().GetPoints()->SetPoint(2, -37.073333, -54.493333, 0);
    observation->specifyCoordinateSystem(coordsSpec);
    model->polyDataSet().GetPoints()->DeepCopy(observation->polyDataSet().GetPoints());
    model->specifyCoordinateSystem(coordsSpec);

    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();
    view = residualView;

    residualView->setModelData(model);
    residualView->setObservationData(observation);
    residualView->waitForResidualUpdate();

    auto residual = residualView->residualData();
    ASSERT_TRUE(residual);
    ASSERT_TRUE(residual->dataSet());
    auto residualScalars = residual->dataSet()->GetCellData()->GetScalars();
    ASSERT_TRUE(residualScalars);
    ASSERT_EQ(1, residualScalars->GetNumberOfComponents());
    ASSERT_EQ(observation->numberOfCells(), residualScalars->GetNumberOfTuples());

    for (vtkIdType i = 0; i < residualScalars->GetNumberOfTuples(); ++i)
    {
        ASSERT_EQ(0.0, residualScalars->GetComponent(i, 0));
    }
}
