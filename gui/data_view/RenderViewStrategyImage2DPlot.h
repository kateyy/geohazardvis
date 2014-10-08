#pragma once

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategyImage2DPlot : public RenderViewStrategy
{
    Q_OBJECT

public:
    RenderViewStrategyImage2DPlot(RenderView & renderView);
    ~RenderViewStrategyImage2DPlot() override;

    QString name() const override;

    void activate() override;
    void deactivate() override;

    bool contains3dData() const override;

    void resetCamera(vtkCamera & camera) override;

    QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects) const override;
    bool canApplyTo(const QList<RenderedData *> & renderedData) override;

private:
    void initialize();

private:
    static const bool s_isRegistered;

    bool m_isInitialized;
};
