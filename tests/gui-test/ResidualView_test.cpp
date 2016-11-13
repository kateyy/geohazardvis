#include <gtest/gtest.h>

#include <array>

#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/utility/DataExtent.h>

#include <gui/DataMapping.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/RendererImplementationResidual.h>

#include "RenderView_test_tools.h"


class ResidualView_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        env = std::make_unique<TestEnv>();
    }
    void TearDown() override
    {
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

    std::unique_ptr<TestEnv> env;

    static std::unique_ptr<PolyDataObject> genPolyData(const QString & name)
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
        scalars->SetNumberOfTuples(poly->GetNumberOfCells());
        for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
        {
            scalars->SetValue(i, static_cast<float>(i));
        }
        poly->GetCellData()->SetScalars(scalars);

        return std::make_unique<PolyDataObject>(name, *poly);
    }
};

TEST_F(ResidualView_test, AbortLoadingDeletedData)
{
    auto observation = genPolyData("observation");
    auto model = genPolyData("displacement vectors");

    auto residualView = env->dataMapping.createRenderView<ResidualVerificationView>();

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

    residualView->setObservationData(obs1.get());
    
    ASSERT_NO_THROW(residualView->setObservationData(obs2.get()));
    ASSERT_EQ(obs2.get(), residualView->observationData());

    env->dataMapping.removeDataObjects({ obs1.get(), obs2.get() });
    env->dataSetHandler.removeExternalData({ obs1.get(), obs2.get() });

    residualView->waitForResidualUpdate();
}
