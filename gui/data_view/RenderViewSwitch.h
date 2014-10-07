#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include <gui/gui_api.h>

class RenderView;
class RenderViewStrategy;
class DataObject;
class RenderedData;


class GUI_API RenderViewSwitch : public QObject
{
    Q_OBJECT

public:
    RenderViewSwitch(RenderView & renderView);
    ~RenderViewSwitch() override;

    const QMap<QString, bool> & applicableStrategies() const;

signals:
    void strategiesChanged(const QMap<QString, bool> & applicableStrategies);

public slots:
    void setStrategy(const QString & name);
    void updateStrategies(const QList<RenderedData *> & renderedData);
    void findSuitableStrategy(const QList<DataObject *> & dataObjects);

private:
    RenderView & m_view;

    QMap<QString, RenderViewStrategy * > m_strategies;
    QMap<QString, bool> m_strategyStates;
};
