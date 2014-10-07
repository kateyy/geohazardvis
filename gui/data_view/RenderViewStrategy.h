#pragma once

#include <functional>

#include <QObject>
#include <QStringList>

#include <gui/gui_api.h>


class vtkCamera;

class RenderView;
class DataObject;
class RenderedData;


class GUI_API RenderViewStrategy : public QObject
{
public:
    RenderViewStrategy(RenderView & renderView);
    virtual ~RenderViewStrategy();

    virtual QString name() const = 0;

    virtual void activate();
    virtual void deactivate();

    virtual bool contains3dData() const = 0;

    /** reset camera view/orientation for new content */
    virtual void resetCamera(vtkCamera & camera) = 0;
    /** reduce list to compatible objects
        @return names of incompatible objects */
    virtual QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects) const = 0;
    
    virtual bool canApplyTo(const QList<RenderedData *> & renderedData) = 0;

    using StategyConstructor = std::function<RenderViewStrategy *(RenderView & view)>;
    static const QList<StategyConstructor> & constructors();

protected:
    template <typename Strategy> static bool registerStrategy();

protected:
    RenderView & m_context;

private:
    static QList<StategyConstructor> & s_constructors();
};

template <typename Strategy>
bool RenderViewStrategy::registerStrategy()
{
    s_constructors().append(
        [] (RenderView & view) { return new Strategy(view); }
    );

    return true;
}
