/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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


class GUI_API RenderViewStrategy : public QObject
{
public:
    explicit RenderViewStrategy(RendererImplementationBase3D & context);
    ~RenderViewStrategy() override;

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

private:
    Q_DISABLE_COPY(RenderViewStrategy)
};

template <typename Strategy>
bool RenderViewStrategy::registerStrategy()
{
    s_constructors().push_back(
        [] (RendererImplementationBase3D & context) { return std::make_unique<Strategy>(context); }
    );

    return true;
}
