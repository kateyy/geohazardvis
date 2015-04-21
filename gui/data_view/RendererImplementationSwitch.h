#pragma once

#include <QList>

class AbstractRenderView;
class DataObject;
class RendererImplementation;


class RendererImplementationSwitch
{
public:
    RendererImplementationSwitch(AbstractRenderView & renderView);
    ~RendererImplementationSwitch();

    void findSuitableImplementation(const QList<DataObject *> & dataObjects);
    RendererImplementation & currentImplementation();

private:
    AbstractRenderView & m_view;
    RendererImplementation * m_currentImpl;
    RendererImplementation * m_nullImpl;
};
