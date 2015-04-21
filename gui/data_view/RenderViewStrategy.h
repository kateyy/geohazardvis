#pragma once

#include <functional>

#include <QObject>
#include <QStringList>

#include <gui/gui_api.h>


class vtkCamera;

class RendererImplementationBase3D;
class DataObject;
class RenderedData;


class GUI_API RenderViewStrategy : public QObject
{
public:
    RenderViewStrategy(RendererImplementationBase3D & context, QObject * parent = nullptr);
    virtual ~RenderViewStrategy();

    virtual QString name() const = 0;

    virtual void activate();
    virtual void deactivate();

    virtual bool contains3dData() const = 0;

    /** reset camera view/orientation for new content */
    virtual void resetCamera(vtkCamera & camera) = 0;
    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const = 0;
    
    virtual bool canApplyTo(const QList<RenderedData *> & renderedData) = 0;

    using StategyConstructor = std::function<RenderViewStrategy *(RendererImplementationBase3D & context)>;
    static const QList<StategyConstructor> & constructors();

protected:
    template <typename Strategy> static bool registerStrategy();

protected:
    RendererImplementationBase3D & m_context;

private:
    static QList<StategyConstructor> & s_constructors();
};

template <typename Strategy>
bool RenderViewStrategy::registerStrategy()
{
    s_constructors().append(
        [] (RendererImplementationBase3D & context) { return new Strategy(context); }
    );

    return true;
}
