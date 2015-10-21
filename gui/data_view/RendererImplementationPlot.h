#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkChartXY;
class vtkContextView;
class vtkObject;
class vtkPlot;
class vtkPlotCollection;

class ChartXY;
class Context2DData;


class GUI_API RendererImplementationPlot : public RendererImplementation
{
public:
    RendererImplementationPlot(AbstractRenderView & renderView);

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & dataObjects) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget & qvtkWidget) override;
    void deactivate(QVTKWidget & qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void setSelectedData(AbstractVisualizedData * vis, vtkIdType index, IndexType indexType) override;
    void setSelectedData(AbstractVisualizedData * vis, vtkIdTypeArray & indices, IndexType indexType) override;
    void clearSelection() override;
    AbstractVisualizedData * selectedData() const override;
    vtkIdType selectedIndex() const override;
    IndexType selectedIndexType() const override;
    void lookAtData(AbstractVisualizedData & vis, vtkIdType index, IndexType indexType, unsigned int subViewIndex) override;
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
    vtkSmartPointer<ChartXY> m_chart;

    // -- contents and annotation --

    bool m_axesAutoUpdate;

    // plots fetched per visualized data
    QMap<Context2DData *, vtkSmartPointer<vtkPlotCollection>> m_plots;

    static bool s_isRegistered;
};
