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

#include <core/CoordinateSystems.h>
#include <gui/data_view/AbstractDataView.h>
#include <gui/data_view/t_QVTKWidgetFwd.h>


class vtkRenderWindow;

class RendererImplementation;


class GUI_API AbstractRenderView : public AbstractDataView
{
    Q_OBJECT

public:
    AbstractRenderView(DataMapping & dataMapping, int index, QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~AbstractRenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    virtual ContentType contentType() const = 0;

    const CoordinateSystemSpecification & currentCoordinateSystem() const;
    /** Switch to the specified coordinate system. If canShowDataObjectInCoordinateSystem() returns
    false, this function will do nothing and also return false. */
    bool setCurrentCoordinateSystem(const CoordinateSystemSpecification & spec);
    bool canShowDataObjectInCoordinateSystem(const CoordinateSystemSpecification & spec);

    /** Add data objects to the view or make already added objects visible again.
        @param incompatibleObjects Objects that cannot be added to the current contents. 
        @param subViewIndex For composed render view, the sub view that the objects will be added to */
    void showDataObjects(
        const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex = 0);
    /** hide rendered representations of data objects, keep render data and settings */
    void hideDataObjects(const QList<DataObject *> & dataObjects, int subViewIndex = -1);
    /** check if the this objects is currently rendered
        @param subViewIndex The index of the sub view in composed render views. -1 means to check for the
        DataObject in any of the sub views */
    bool contains(DataObject * dataObject, int subViewIndex = -1) const;
    /** Remove rendered representations and all references to the data objects.
        For composed views, all sub views will be cleared from the specified data objects */
    void prepareDeleteData(const QList<DataObject *> & dataObjects);
    /** DataObjects visible in the specified subViewIndex, or in any of the sub views (-1) */
    QList<DataObject *> dataObjects(int subViewIndex = -1) const;
    QList<AbstractVisualizedData *> visualizations(int subViewIndex = -1) const;
    /** @return visualization of dataObject in the specified sub-view. Returns nullptr, if the view currently doesn't visualize this data object*/
    virtual AbstractVisualizedData * visualizationFor(DataObject * dataObject, int subViewIndex = -1) const = 0;
    /** @return the sub view index that contains the specified visualization, or -1 if this is not part of this view */
    virtual int subViewContaining(const AbstractVisualizedData & visualizedData) const = 0;

    virtual bool isEmpty() const = 0;

    /** More specialized variant of AbstractDataView::setSelection
      * Compared to the base class function, this allows also allows to specify a visualization
      * and its output port. */
    void setVisualizationSelection(const VisualizationSelection & selection);
    const VisualizationSelection & visualzationSelection() const;

    virtual void lookAtData(const DataSelection & selection, int subViewIndex = -1) = 0;
    virtual void lookAtData(const VisualizationSelection & selection, int subViewIndex = -1) = 0;

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;

    virtual RendererImplementation & implementation() const = 0;

    /** show/hide axes in case the render view currently contains data */
    void setEnableAxes(bool enabled);
    bool axesEnabled() const;

    void render();

    void showInfoText(const QString & text);
    QString infoText() const;

    /** Set a function that is called each time an info text is required.
    * This can be used to implement the text update in a pull instead of a push style. */
    void setInfoTextCallback(std::function<QString()> callback);

signals:
    /** emitted after changing the list of visible objects */
    void visualizationsChanged();

    /** Emitted after switching to a now implementation. 
    * Do not access pointers to the previous implementation when receiving this signal! */
    void implementationChanged();

    void visualizationSelectionChanged(AbstractRenderView * renderView, const VisualizationSelection & selection);

    void currentCoordinateSystemChanged(const CoordinateSystemSpecification & spec);

    void beforeDeleteVisualizations(const QList<AbstractVisualizedData *> & visualizations);

protected:
    QWidget * contentWidget() override;
    t_QVTKWidget & qvtkWidget();

    void showEvent(QShowEvent * event) override;
    void paintEvent(QPaintEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;

    void onSetSelection(const DataSelection & selection) override;
    void onClearSelection() override;
    
    std::pair<QString, std::vector<QString>> friendlyNameInternal() const override;

    /** Called once the first time the widget is shown.
      * Initialize OpenGL contexts in function so that it is not done before the application setup.*/
    virtual void initializeRenderContext() = 0;

    virtual void onCoordinateSystemChanged(const CoordinateSystemSpecification & spec);

    /** Called when the current selection is changed (set or clear) */
    virtual void visualizationSelectionChangedEvent(const VisualizationSelection & selection);

    virtual void showDataObjectsImpl(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects,
        unsigned int subViewIndex) = 0;
    virtual void hideDataObjectsImpl(const QList<DataObject *> & dataObjects, int subViewIndex) = 0;
    virtual QList<DataObject *> dataObjectsImpl(int subViewIndex) const = 0;
    virtual void prepareDeleteDataImpl(const QList<DataObject *> & dataObjects) = 0;
    virtual QList<AbstractVisualizedData *> visualizationsImpl(int subViewIndex) const = 0;

private:
    void initializeForFirstPaint();

private:
    t_QVTKWidget * m_qvtkWidget;
    bool m_onFirstPaintInitialized;

    CoordinateSystemSpecification m_coordSystem;

    bool m_axesEnabled;

    QMetaObject::Connection m_infoTextConnection;
    std::function<QString()> m_infoTextCallback;

private:
    Q_DISABLE_COPY(AbstractRenderView)
};
