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

#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkChartXY;
class vtkContextView;
class vtkGenericOpenGLRenderWindow;
class vtkObject;
class vtkPlot;
class vtkPlotCollection;

class ChartXY;
class Context2DData;


class GUI_API RendererImplementationPlot : public RendererImplementation
{
public:
    explicit RendererImplementationPlot(AbstractRenderView & renderView);
    ~RendererImplementationPlot() override;

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & dataObjects) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(t_QVTKWidget & qvtkWidget) override;
    void deactivate(t_QVTKWidget & qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void lookAtData(const VisualizationSelection & selection, unsigned int subViewIndex) override;
    void resetCamera(bool toInitialPosition, unsigned int subViewIndex) override;

    void setAxesVisibility(bool visible) override;

    /** automatically update axes for changed chart contents */
    bool axesAutoUpdate() const;
    void setAxesAutoUpdate(bool enable);

    vtkChartXY * chart();
    vtkContextView * contextView();

    std::unique_ptr<AbstractVisualizedData> requestVisualization(DataObject & dataObject) const override;

protected:
    void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;

    void onSetSelection(const VisualizationSelection & selection) override;
    void onClearSelection() override;

    Context2DData * contextDataContaining(const vtkPlot & plot) const;

private:
    void initialize();

    void updateBounds();

    /** scan data for changed context items */
    void fetchContextItems(Context2DData * data);

    void dataVisibilityChanged(Context2DData * data);

    void handlePlotSelectionEvent(vtkObject * subject, unsigned long eventId, void * callData);

private:
    bool m_isInitialized;

    // -- setup --
    vtkSmartPointer<vtkContextView> m_contextView;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<ChartXY> m_chart;

    // -- contents and annotation --

    bool m_axesAutoUpdate;

    // plots fetched per visualized data
    QMap<Context2DData *, vtkSmartPointer<vtkPlotCollection>> m_plots;

    Context2DData * m_selectedPlot;

    static bool s_isRegistered;

private:
    Q_DISABLE_COPY(RendererImplementationPlot)
};
