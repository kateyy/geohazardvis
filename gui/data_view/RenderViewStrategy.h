#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QObject>

#include <vtkSmartPointer.h>

#include <gui/gui_api.h>


class vtkCamera;

class RendererImplementationBase3D;
class DataObject;
class DataMapping;
class RenderedData;


class GUI_API RenderViewStrategy : public QObject
{
public:
    explicit RenderViewStrategy(RendererImplementationBase3D & context);
    virtual ~RenderViewStrategy();

    DataMapping & dataMapping() const;

    virtual QString name() const = 0;

    /** Called from the context before a strategy instance is used. */
    void activate();
    /** Called from the context when a strategy instance is not used anymore. */
    void deactivate();

    virtual bool contains3dData() const = 0;

    /** @param dataObjects Check if we can render these objects with the current view content.
        @param incompatibleObjects Fills this list with discarded objects.
        @return compatible objects that we will render. */
    virtual QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const = 0;

    using StategyConstructor = std::function<std::unique_ptr<RenderViewStrategy> (RendererImplementationBase3D & context)>;
    static const std::vector<StategyConstructor> & constructors();

protected:
    template <typename Strategy> static bool registerStrategy();

    /** Name of the interactor style that is used by default by subclasses.
      * This style will be activated in activate() */
    virtual QString defaultInteractorStyle() const;

    virtual void onActivateEvent();
    virtual void onDeactivateEvent();

    /** @brief Restores sets the first render view's camera to a default position(on first activate()) or resets 
      * it to its state at the last deactivate() call. 
      *
      * Automatically called in activate()
      * @see m_storedCamera */
    void restoreCamera();
    /** Stores the current state of the first sub-view's camera.
      * Automatically called in deactivate()*/
    void backupCamera();

protected:
    RendererImplementationBase3D & m_context;

    /** vtkCamera instance that is used to backup the camera configuration on backup. */
    vtkSmartPointer<vtkCamera> m_storedCamera;

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
