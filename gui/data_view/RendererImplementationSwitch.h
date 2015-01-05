#pragma once

#include <QMap>
#include <QObject>
#include <QString>

#include <gui/gui_api.h>

class RenderView;
class RendererImplementation;
class DataObject;


class GUI_API RendererImplementationSwitch : public QObject
{
    Q_OBJECT

public:
    RendererImplementationSwitch(RenderView & renderView);
    ~RendererImplementationSwitch() override;

public slots:
    void setImplementation(const QString & name);
    void findSuitableImplementation(const QList<DataObject *> & dataObjects);

private:
    RenderView & m_view;

    QMap<QString, RendererImplementation *> m_implementations;
};
