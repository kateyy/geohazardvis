#pragma once

#include <QDockWidget>
#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include <core/data_mapping/ScalarToColorMapping.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkPolyData;
class vtkCubeAxesActor;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkActor;

class DataObject;
class RenderedData;

class DataChooser;
class NormalRepresentation;
class PickingInteractorStyleSwitch;
class IPickingInteractorStyle;
class RenderConfigWidget;
class Ui_RenderView;


class RenderView : public QDockWidget
{
    Q_OBJECT

public:
    RenderView(
        int index,
        DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget);
    ~RenderView() override;

    int index() const;

    /** add data objects to the view or make already added objects visible again */
    void addDataObjects(QList<DataObject *> dataObjects);
    /** remove rendered representations of data objects, don't delete data and settings */
    void hideDataObjects(QList<DataObject *> dataObjects);
    /** check if the this objects is currently rendered */
    bool isVisible(DataObject * dataObject) const;
    /** remove rendered representations and all references to the data objects */
    void removeDataObjects(QList<DataObject *> dataObjects);
    QList<DataObject *> dataObjects() const;
    QList<const RenderedData *> renderedData() const;

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;
    
    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;

public slots:
    void render();

    void ShowInfo(const QStringList &info);

signals:
    void closed();
    /** signaled when the widget receive the keyboard focus (focusInEvent) */
    void focused(RenderView * renderView);

protected:
    void focusInEvent(QFocusEvent * event);
    void focusOutEvent(QFocusEvent * event);

    bool eventFilter(QObject * obj, QEvent * ev) override;

private:
    void setupRenderer();
    void setupInteraction();
    void setInteractorStyle(const std::string & name);

    RenderedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    void updateWindowTitle();
    
    void updateVertexNormals(vtkPolyData * polyData);
    
    void setupAxes(const double bounds[6]);
    static vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
    void setupColorMappingLegend();

    void closeEvent(QCloseEvent * event) override;

private slots:
    /** Updates the RenderConfigWidget to reflect the actors render properties. */
    void updateGuiForActor(vtkActor * actor);

private:
    Ui_RenderView * m_ui;

    const int m_index;

    // rendered representations of data objects for this view
    QList<RenderedData *> m_renderedData;
    QMap<DataObject *, RenderedData *> m_dataObjectToRendered;
    QMap<vtkActor *, RenderedData *> m_actorToRenderedData;

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;

    // visualization and annotation
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
    
    // configuration widgets
    DataChooser & m_dataChooser;
    ScalarToColorMapping m_scalarMapping;
    RenderConfigWidget & m_renderConfigWidget;
};
