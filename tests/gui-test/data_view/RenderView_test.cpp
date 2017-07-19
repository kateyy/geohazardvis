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

#include <array>

#include <QCoreApplication>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/DataProfile2DDataObject.h>
#include <core/utility/DataExtent.h>

#include <gui/DataMapping.h>
#include <gui/data_view/RenderView.h>
#include <gui/data_view/RendererImplementationBase3D.h>
#include <gui/data_view/RenderViewStrategy2D.h>

#include "RenderView_test_tools.h"


class RenderView_test : public ::testing::Test
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
        auto indices = vtkSmartPointer<vtkCellArray>::New();

        // texture mapping produces warnings if the data set does not contain at least 3 points
        points->InsertNextPoint(0, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(1, 1, 0);
        std::array<vtkIdType, 3> pointIds = { 0, 1, 2 };
        indices->InsertNextCell(3, pointIds.data());
        poly->SetPoints(points);
        poly->SetPolys(indices);

        return std::make_unique<PolyDataObject>("PolyData", *poly);
    }

    struct ProfileData
    {
        std::unique_ptr<PolyDataObject> sourceData;
        std::unique_ptr<DataProfile2DDataObject> profile;

        ProfileData() = default;
        ProfileData(ProfileData && other)
            : sourceData{ std::move(other.sourceData) }
            , profile{ std::move(other.profile) }
        {
        }
        ProfileData & operator=(ProfileData && other)
        {
            sourceData = std::move(other.sourceData);
            profile = std::move(other.profile);
            return *this;
        }
    };

    static ProfileData genProfileData()
    {
        ProfileData profileData;
        profileData.sourceData = genPolyData();

        auto scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetName("scalars");
        scalars->SetNumberOfTuples(profileData.sourceData->numberOfCells());
        for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
        {
            scalars->SetValue(i, static_cast<float>(i));
        }
        profileData.sourceData->dataSet()->GetCellData()->AddArray(scalars);

        profileData.profile = std::make_unique<DataProfile2DDataObject>(
            "Profile", *profileData.sourceData,
            "scalars", IndexType::cells, 0);

        const auto inBounds = DataBounds(profileData.sourceData->bounds()).convertTo<2>();
        profileData.profile->setProfileLinePoints(inBounds.min(), inBounds.max());

        return profileData;
    }
};

/**
 * The following three tests cover bugs that were cased by the new QVTKOpenGLWidget.
 * However, they are still a good baseline for correct behavior of RenderView.
 * (InitEmptyView, InitViewWithContents, SwitchImplementation)
 */
TEST_F(RenderView_test, InitEmptyView)
{
    auto view = env->dataMapping.createDefaultRenderViewType();
    ASSERT_TRUE(dynamic_cast<RenderView *>(view));
    view->show();
    ASSERT_NO_THROW(qApp->processEvents());
}

TEST_F(RenderView_test, InitViewWithContents)
{
    auto ownedPoly = genPolyData();
    auto poly = ownedPoly.get();
    env->dataSetHandler.takeData(std::move(ownedPoly));
    auto view = env->dataMapping.openInRenderView({ poly });
    ASSERT_TRUE(dynamic_cast<RenderView *>(view));
    view->show();
    ASSERT_NO_THROW(qApp->processEvents());
}

TEST_F(RenderView_test, SwitchImplementation)
{
    auto ownedPoly = genPolyData();
    auto poly = ownedPoly.get();
    env->dataSetHandler.takeData(std::move(ownedPoly));
    auto view = env->dataMapping.createDefaultRenderViewType();
    ASSERT_TRUE(dynamic_cast<RenderView *>(view));
    view->show();
    ASSERT_NO_THROW(qApp->processEvents());

    QList<DataObject *> incompatible;
    view->showDataObjects({ poly }, incompatible);
    ASSERT_TRUE(incompatible.isEmpty());
    ASSERT_NO_THROW(qApp->processEvents());
}

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

TEST_F(RenderView_test, UnselectDeletedData)
{
    auto polyData = genPolyData();

    env->dataSetHandler.addExternalData({ polyData.get() });
    auto renderView = env->dataMapping.openInRenderView({ polyData.get() });

    env->dataMapping.removeDataObjects({ polyData.get() });
    env->dataSetHandler.removeExternalData({ polyData.get() });
    polyData.reset();

    auto selection = renderView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);
}

TEST_F(RenderView_test, UnselectHiddendData)
{
    auto polyData = genPolyData();

    env->dataSetHandler.addExternalData({ polyData.get() });
    auto renderView = env->dataMapping.openInRenderView({ polyData.get() });

    renderView->hideDataObjects({ polyData.get() });

    auto selection = renderView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);

    env->dataMapping.removeDataObjects({ polyData.get() });
    env->dataSetHandler.removeExternalData({ polyData.get() });
}

TEST_F(RenderView_test, UnselectDeletedProfileData)
{
    auto profileData = genProfileData();
    auto profile = profileData.profile.get();

    env->dataSetHandler.addExternalData({ profile });
    auto renderView = env->dataMapping.openInRenderView({ profile });

    env->dataMapping.removeDataObjects({ profile });
    env->dataSetHandler.removeExternalData({ profile });
    profileData = {};

    auto selection = renderView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);
}

TEST_F(RenderView_test, UnselectHiddendProfileData)
{
    auto profileData = genProfileData();
    auto profile = profileData.profile.get();

    env->dataSetHandler.addExternalData({ profile });
    auto renderView = env->dataMapping.openInRenderView({ profile });

    renderView->hideDataObjects({ profile });

    auto selection = renderView->selection();
    ASSERT_EQ(nullptr, selection.dataObject);

    env->dataMapping.removeDataObjects({ profile });
    env->dataSetHandler.removeExternalData({ profile });
}
