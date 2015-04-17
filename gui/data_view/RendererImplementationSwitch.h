#pragma once

#include <QList>

class DataObject;
class RendererImplementation;
class RenderView;


class RendererImplementationSwitch
{
public:
    RendererImplementationSwitch(RenderView & renderView);
    ~RendererImplementationSwitch();

    void findSuitableImplementation(const QList<DataObject *> & dataObjects);
    RendererImplementation & currentImplementation();

private:
    RenderView & m_view;
    RendererImplementation * m_currentImpl;
    RendererImplementation * m_nullImpl;
};
