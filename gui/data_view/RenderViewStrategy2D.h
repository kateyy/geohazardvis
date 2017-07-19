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

#include <map>
#include <vector>

#include <core/CoordinateSystems_fwd.h>
#include <gui/data_view/RenderViewStrategy.h>


class QAction;
class vtkObject;
class vtkLineWidget2;

class AbstractRenderView;


class GUI_API RenderViewStrategy2D : public RenderViewStrategy
{
public:
    explicit RenderViewStrategy2D(RendererImplementationBase3D & context);
    ~RenderViewStrategy2D() override;

    /** Explicitly define a list of images to create a profile plot for. 
        The same plot line will be applied to all images.
        When specifying an empty list here, the first image contained in the Strategy's context will be used. */
    void setInputData(const QList<DataObject *> & images);

    QString name() const override;

    bool contains3dData() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

    /** Start or refresh the current profile plot, open a new preview renderer if required. */
    void startProfilePlot();
    void acceptProfilePlot();
    void abortProfilePlot();

    /** @return the render view that is created to visualize the profile plot.
      * This is nullptr, if no plot has been started, no valid input data is available, or a
      * previous plot as been accepted. */
    AbstractRenderView * plotPreviewRenderer();

protected:
    QString defaultInteractorStyle() const override;

    void onActivateEvent() override;
    void onDeactivateEvent() override;

private:
    void initialize();

    /** Delete current plots, but do not change the GUI state */
    void clearProfilePlots();
    void lineMoved();
    void updateAutomaticPlots();
    void updateForViewCoordinateSystemChange(const CoordinateSystemSpecification & spec);

private:
    static const bool s_isRegistered;

    enum class State
    {
        uninitialized, notPlotting, plotSetup, plotting, accepting
    };
    State m_state;

    QAction * m_profilePlotAction;
    QAction * m_profilePlotAcceptAction;
    QAction * m_profilePlotAbortAction;
    QList<QAction *> m_actions;
    std::vector<std::unique_ptr<DataObject>> m_previewProfiles;
    AbstractRenderView * m_previewRenderer;
    std::vector<QMetaObject::Connection> m_previewRendererConnections;

    QList<DataObject *> m_activeInputData;  // currently used input data
    QList<DataObject *> m_inputData;        // input data that was explicitly set

    vtkSmartPointer<vtkLineWidget2> m_lineWidget;
    std::multimap<vtkSmartPointer<vtkObject>, unsigned long> m_observerTags;
    bool m_pausePointsUpdate;

private:
    Q_DISABLE_COPY(RenderViewStrategy2D)
};
