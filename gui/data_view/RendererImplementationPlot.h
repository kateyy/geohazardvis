#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <gui/data_view/RendererImplementation.h>


class vtkChartXY;
class vtkContextView;
class vtkPlotCollection;

class Context2DData;


class RendererImplementationPlot : public RendererImplementation
{
    Q_OBJECT

public:
    RendererImplementationPlot(AbstractRenderView & renderView, QObject * parent = nullptr);

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & dataObjects) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget * qvtkWidget) override;
    void deactivate(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void setSelectedData(DataObject * dataObject, vtkIdType itemId = -1) override;
    virtual DataObject * selectedData() const override;
    vtkIdType selectedIndex() const override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    void setAxesVisibility(bool visible) override;

    /** automatically update axes for changed chart contents */
    bool axesAutoUpdate() const;
    void setAxesAutoUpdate(bool enable);

    vtkChartXY * chart();
    vtkContextView * contextView();

protected:
    AbstractVisualizedData * requestVisualization(DataObject * dataObject) const override;

    void onAddContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;

private:
    void initialize();

    void updateBounds();

private slots:
    /** scan data for changed context items */
    void fetchContextItems(Context2DData * data);

    void dataVisibilityChanged(Context2DData * data);

private:
    bool m_isInitialized;

    // -- setup --
    vtkSmartPointer<vtkContextView> m_contextView;
    vtkSmartPointer<vtkChartXY> m_chart;

    // -- contents and annotation --

    bool m_axesAutoUpdate;

    // plots fetched per visualized data
    QMap<Context2DData *, vtkSmartPointer<vtkPlotCollection>> m_plots;

    static bool s_isRegistered;
};
