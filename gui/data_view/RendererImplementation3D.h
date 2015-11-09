#pragma once

#include <map>
#include <memory>

#include <gui/data_view/RendererImplementationBase3D.h>


class RenderViewStrategySwitch;


class GUI_API RendererImplementation3D : public RendererImplementationBase3D
{
public:
    explicit RendererImplementation3D(AbstractRenderView & renderView);
    ~RendererImplementation3D() override;

    QString name() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget & qvtkWidget) override;

protected:
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;

    ColorMapping * colorMappingForSubView(unsigned int subViewIndex) override;

    void updateForCurrentInteractionStrategy(const QString & strategyName) override;
    RenderViewStrategy * strategyIfEnabled() const override;


private:
    void updateStrategies(const QList<DataObject *> & newDataObjects = {});
    QString mostSuitableStrategy(const QList<DataObject *> & newDataObjects) const;

private:
    std::map<QString, std::unique_ptr<RenderViewStrategy>> m_strategies;
    RenderViewStrategy * m_currentStrategy;
    std::unique_ptr<ColorMapping> m_colorMapping;

    static bool s_isRegistered;
};
