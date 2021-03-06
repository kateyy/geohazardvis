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

#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <core/utility/vtkVector_print.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementation3D.h>
#include <gui/data_view/RenderViewStrategy.h>

#include "RenderView_test_tools.h"
#include "TestDataExtent.h"


class TestRendererImplementation3D : public RendererImplementation3D
{
public:
    using RendererImplementation3D::RendererImplementation3D;
    using RendererImplementation3D::strategy;
};

template<ContentType contentTypeT>
class TestRenderView : public AbstractRenderView
{
public:
    TestRenderView(DataMapping & dataMapping, int index = 0)
        : AbstractRenderView(dataMapping, index)
        , m_impl{ std::make_unique<TestRendererImplementation3D>(*this) }
    {
        m_impl->activate(qvtkWidget());
    }
    ~TestRenderView() override
    {
        signalClosing();
        m_impl->deactivate(qvtkWidget());
    }

    ContentType contentType() const override { return contentTypeT; }

    AbstractVisualizedData * visualizationFor(DataObject * dataObject, int /*subViewIndex*/ = -1) const override
    {
        for (auto & vis : m_vis)
        {
            if (&vis->dataObject() == dataObject)
            {
                return vis.get();
            }
        }
        return nullptr;
    }

    bool isEmpty() const override
    {
        return m_vis.empty();
    }

    int subViewContaining(const AbstractVisualizedData & /*visualizedData*/) const override { return 0; }
    void lookAtData(const DataSelection & /*selection*/, int /*subViewIndex*/ = -1) override {}
    void lookAtData(const VisualizationSelection & /*selection*/, int /*subViewIndex*/ = -1) override {}

    RendererImplementation & implementation() const override { return *m_impl; }
    TestRendererImplementation3D & impl() { return *m_impl; }

    RenderViewStrategy & strategy() { return impl().strategy(); }

protected:
    void initializeRenderContext() override { }

    void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int /*subViewIndex*/) override
    {
        const auto compatible = m_impl->filterCompatibleObjects(dataObjects, incompatibleObjects);
        if (compatible.isEmpty())
        {
            return;
        }

        for (auto dataObject : compatible)
        {
            auto vis = m_impl->requestVisualization(*dataObject);
            auto visPtr = vis.get();
            m_vis.push_back(std::move(vis));

            m_impl->addContent(visPtr, 0);
        }

        m_impl->renderViewContentsChanged();
    }

    void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int /*subViewIndex*/) override
    {
        prepareDeleteDataImpl(dataObjects);
    }
    QList<DataObject *> dataObjectsImpl(int /*subViewIndex*/) const override
    {
        QList<DataObject *> dataObjs;
        for (auto & vis : m_vis)
        {
            dataObjs << &vis->dataObject();
        }
        return dataObjs;
    }
    void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) override
    {
        std::vector<std::unique_ptr<AbstractVisualizedData>> toDelete;
        for (auto dataObject : dataObjects)
        {
            for (auto visIt = m_vis.begin(); visIt != m_vis.end(); ++visIt)
            {
                if (&(*visIt)->dataObject() != dataObject)
                {
                    continue;
                }
                const auto visPtr = visIt->get();
                toDelete.push_back(std::move(*visIt));
                m_vis.erase(visIt);
                m_impl->removeContent(visPtr, 0);
                break;
            }
        }
        m_impl->renderViewContentsChanged();
    }
    QList<AbstractVisualizedData *> visualizationsImpl(int /*subViewIndex*/) const override
    {
        QList<AbstractVisualizedData *> visList;
        for (auto & vis : m_vis)
        {
            visList << vis.get();
        }
        return visList;
    }

private:
    std::unique_ptr<TestRendererImplementation3D> m_impl;

    std::vector<std::unique_ptr<AbstractVisualizedData>> m_vis;
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_visCache;
};




class RendererImplementation3D_test : public ::testing::Test
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
        {
        }

        DataSetHandler dataSetHandler;
        DataMapping dataMapping;
    };

    std::unique_ptr<TestEnv> env;

    template<typename T = PolyDataObject>
    static std::unique_ptr<T> genPolyData2D()
    {
        auto poly = vtkSmartPointer<vtkPolyData>::New();
        auto points = vtkSmartPointer<vtkPoints>::New();

        points->InsertNextPoint(0, 0, 0);
        points->InsertNextPoint(0, 1, 0);
        points->InsertNextPoint(1, 1, 0);
        poly->SetPoints(points);
        std::array<vtkIdType, 3> pointIds = { 0, 1, 2 };
        poly->Allocate(static_cast<vtkIdType>(pointIds.size()));
        poly->InsertNextCell(VTK_TRIANGLE, static_cast<int>(pointIds.size()), pointIds.data());

        return std::make_unique<T>("PolyData", *poly);
    }

    template<typename T = PolyDataObject>
    static std::unique_ptr<T> genPolyData3D()
    {
        auto data = genPolyData2D();
        data->polyDataSet().GetPoints()->SetPoint(2, 1, 1, 0.1);
        data->polyDataSet().Modified();
        return data;
    }
};

TEST_F(RendererImplementation3D_test, Strategy3dFor3dData)
{
    auto data = genPolyData3D();

    TestRenderView<ContentType::Rendered3D> renderView(env->dataMapping);
    QList<DataObject *> incompatible;
    renderView.showDataObjects({ data.get() }, incompatible);
    ASSERT_TRUE(incompatible.isEmpty());

    ASSERT_STREQ("3D terrain", renderView.strategy().name().toUtf8().data());
}

TEST_F(RendererImplementation3D_test, Strategy2dFor2dData)
{
    auto data = genPolyData2D();

    TestRenderView<ContentType::Rendered3D> renderView(env->dataMapping);
    QList<DataObject *> incompatible;
    renderView.showDataObjects({ data.get() }, incompatible);
    ASSERT_TRUE(incompatible.isEmpty());

    ASSERT_STREQ("2D image", renderView.strategy().name().toUtf8().data());
}
