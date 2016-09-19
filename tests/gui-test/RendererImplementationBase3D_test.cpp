#include <gtest/gtest.h>

#include <vtkPolyData.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedPolyData.h>
#include <core/ThirdParty/ParaView/vtkGridAxes3DActor.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/RendererImplementation3D.h>

#include "RenderView_test_tools.h"


template<typename ImplT, ContentType contentTypeT>
class TestRenderView : public AbstractRenderView
{
public:
    TestRenderView(DataMapping & dataMapping, int index = 0)
        : AbstractRenderView(dataMapping, index)
        , m_impl(std::make_unique<ImplT>(*this))
    {
        m_impl->activate(qvtkWidget());
    }
    ~TestRenderView() override
    {
        m_impl->deactivate(qvtkWidget());
    }

    QString friendlyName() const override { return "TestRenderView"; }

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

    int subViewContaining(const AbstractVisualizedData & /*visualizedData*/) const override { return 0; }
    void lookAtData(const DataSelection & /*selection*/, int /*subViewIndex*/ = -1) override {}
    void lookAtData(const VisualizationSelection & /*selection*/, int /*subViewIndex*/ = -1) override {}

    RendererImplementation & implementation() const override { return *m_impl; }
    ImplT & impl() const { return *m_impl; }

protected:
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
        for (auto dataObject : dataObjects)
        {
            for (auto visIt = m_vis.begin(); visIt != m_vis.end(); ++visIt)
            {
                if (&(*visIt)->dataObject() != dataObject)
                {
                    continue;
                }
                const auto visPtr = visIt->get();
                m_visCache.push_back(std::move(*visIt));
                m_vis.erase(visIt);
                m_impl->removeContent(visPtr, 0);
                break;
            }
        }
        m_impl->renderViewContentsChanged();
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
            for (auto visIt = m_visCache.begin(); visIt != m_visCache.end(); ++visIt)
            {
                if (&(*visIt)->dataObject() != dataObject)
                {
                    continue;
                }
                const auto visPtr = visIt->get();
                toDelete.push_back(std::move(*visIt));
                m_visCache.erase(visIt);
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
    void axesEnabledChangedEvent(bool enabled) override
    {
        m_impl->setAxesVisibility(!m_vis.empty() && enabled);
    }

private:
    std::unique_ptr<ImplT> m_impl;

    std::vector<std::unique_ptr<AbstractVisualizedData>> m_vis;
    std::vector<std::unique_ptr<AbstractVisualizedData>> m_visCache;
};


class ShiftedRenderedPolyData : public RenderedPolyData
{
public:
    explicit ShiftedRenderedPolyData(PolyDataObject & dataObject)
        : RenderedPolyData(dataObject)
    {
    }

    DataBounds updateVisibleBounds() override
    {
        auto bounds = RenderedPolyData::visibleBounds();

        for (size_t i = 0; i < bounds.ValueCount; ++i)
        {
            bounds[i] += 100.0;
        }

        return bounds;
    }

    const DataBounds & originalVisibleBounds()
    {
        return RenderedPolyData::visibleBounds();
    }
};

class ShiftedPolyData : public PolyDataObject
{
public:
    explicit ShiftedPolyData(const QString & name, vtkPolyData & dataSet)
        : PolyDataObject(name, dataSet)
    {
    }

    std::unique_ptr<RenderedData> createRendered() override
    {
        return std::make_unique<ShiftedRenderedPolyData>(*this);
    }
};


class RendererImplementationBase3D_test : public testing::Test
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
    static std::unique_ptr<T> genPolyData()
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
};

TEST_F(RendererImplementationBase3D_test, GridAxesBoundsSetToVisibleBounds)
{
    auto data = genPolyData<PolyDataObject>();

    TestRenderView<RendererImplementation3D, ContentType::Rendered3D> renderView(env->dataMapping);
    QList<DataObject *> incompatible;
    renderView.showDataObjects({ data.get() }, incompatible);
    ASSERT_TRUE(incompatible.isEmpty());
    auto vis = renderView.visualizationFor(data.get());
    ASSERT_TRUE(vis);
    auto rendered = dynamic_cast<RenderedData3D *>(vis);
    ASSERT_TRUE(rendered);

    const auto visibleBounds = rendered->visibleBounds();
    const auto axesBounds = DataBounds(renderView.impl().axesActor(0)->GetBounds());

    ASSERT_EQ(visibleBounds, axesBounds);
}

TEST_F(RendererImplementationBase3D_test, GridAxesBoundsSetToShiftedVisibleBounds)
{
    auto data = genPolyData<ShiftedPolyData>();

    TestRenderView<RendererImplementation3D, ContentType::Rendered3D> renderView(env->dataMapping);
    QList<DataObject *> incompatible;
    renderView.showDataObjects({ data.get() }, incompatible);
    ASSERT_TRUE(incompatible.isEmpty());
    auto vis = renderView.visualizationFor(data.get());
    ASSERT_TRUE(vis);
    auto rendered = dynamic_cast<RenderedData3D *>(vis);
    ASSERT_TRUE(rendered);

    const auto visibleBounds = rendered->visibleBounds();
    const auto axesBounds = DataBounds(renderView.impl().axesActor(0)->GetBounds());

    ASSERT_EQ(visibleBounds, axesBounds);
}
