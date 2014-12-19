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
    RendererImplementationPlot(RenderView & renderView, QObject * parent = nullptr);
    ~RendererImplementationPlot() override;

    QString name() const override;

    ContentType contentType() const override;

    bool canApplyTo(const QList<DataObject *> & dataObjects) override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) override;

    void activate(QVTKWidget * qvtkWidget) override;

    void render() override;

    vtkRenderWindowInteractor * interactor() override;

    void addContent(AbstractVisualizedData * content) override;
    void removeContent(AbstractVisualizedData * content) override;

    void highlightData(DataObject * dataObject, vtkIdType itemId = -1) override;
    virtual DataObject * highlightedData() override;
    void lookAtData(DataObject * dataObject, vtkIdType itemId) override;
    void resetCamera(bool toInitialPosition) override;

    void setAxesVisibility(bool visible) override;

protected:
    AbstractVisualizedData * requestVisualization(DataObject * dataObject) const override;

private:
    void initialize();

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

    // plots fetched per visualized data
    QMap<Context2DData *, vtkSmartPointer<vtkPlotCollection>> m_plots;

    static bool s_isRegistered;
};
