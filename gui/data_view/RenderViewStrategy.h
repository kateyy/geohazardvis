#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QObject>

#include <gui/gui_api.h>


class vtkCamera;

class RendererImplementationBase3D;
class DataObject;
class RenderedData;


class GUI_API RenderViewStrategy : public QObject
{
public:
    RenderViewStrategy(RendererImplementationBase3D & context);
    virtual ~RenderViewStrategy();

    virtual QString name() const = 0;

    virtual void activate();
    virtual void deactivate();

    virtual bool contains3dData() const = 0;

    /** Reset the camera of the current renderer to the default position.
      * (The current renderer is defined in the render view's vtkRenderWindowInteractor) */
    virtual void resetCamera() = 0;
    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const = 0;
    
    virtual bool canApplyTo(const QList<RenderedData *> & renderedData) = 0;

    using StategyConstructor = std::function<std::unique_ptr<RenderViewStrategy> (RendererImplementationBase3D & context)>;
    static const std::vector<StategyConstructor> & constructors();

protected:
    template <typename Strategy> static bool registerStrategy();

protected:
    RendererImplementationBase3D & m_context;

private:
    static std::vector<StategyConstructor> & s_constructors();
};

template <typename Strategy>
bool RenderViewStrategy::registerStrategy()
{
    s_constructors().push_back(
        [] (RendererImplementationBase3D & context) { return std::make_unique<Strategy>(context); }
    );

    return true;
}
