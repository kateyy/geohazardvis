#pragma once

#include <memory>

#include <gui/data_view/RendererImplementationBase3D.h>


class RendererImplementation3D : public RendererImplementationBase3D
{
    Q_OBJECT

public:
    RendererImplementation3D(AbstractRenderView & renderView, QObject * parent = nullptr);
    ~RendererImplementation3D() override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget * qvtkWidget) override;

signals:
    void resetStrategy(const QList<DataObject *> & dataObjects);

protected:
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onDataVisibilityChanged(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onRenderViewVisualizationChanged();

private:
    std::unique_ptr<RenderViewStrategySwitch> m_strategySwitch;

    static bool s_isRegistered;
};