#pragma once

#include <QMap>
#include <QObject>

#include <gui/gui_api.h>


class QString;

class RendererImplementation3D;
class RenderViewStrategy;
class DataObject;
class RenderedData;


class GUI_API RenderViewStrategySwitch : public QObject
{
    Q_OBJECT

public:
    RenderViewStrategySwitch(RendererImplementation3D & renderView, QObject * parent = nullptr);
    ~RenderViewStrategySwitch() override;

    const QMap<QString, bool> & applicableStrategies() const;

signals:
    void strategiesChanged(const QMap<QString, bool> & applicableStrategies);

public slots:
    void setStrategy(const QString & name);
    void updateStrategies();
    void findSuitableStrategy(const QList<DataObject *> & dataObjects);

private:
    RendererImplementation3D & m_view;

    QMap<QString, RenderViewStrategy * > m_strategies;
    QMap<QString, bool> m_strategyStates;
};
