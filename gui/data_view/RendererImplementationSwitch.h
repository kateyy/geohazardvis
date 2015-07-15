#pragma once

#include <memory>


template<typename T> class QList;

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
    std::unique_ptr<RendererImplementation> m_currentImpl;
    std::unique_ptr<RendererImplementation> m_nullImpl;
};
