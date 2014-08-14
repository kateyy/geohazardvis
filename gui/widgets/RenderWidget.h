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
class Ui_RenderWidget;


class RenderWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderWidget(
        int index,
        DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget);
    ~RenderWidget() override;

    int index() const;

    void addDataObject(DataObject * dataObject);
    void setDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);
    QList<DataObject *> dataObjects() const;

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;
    
    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;

public slots:
    void render();

    void ShowInfo(const QStringList &info);

signals:
    void closed();

private:
    void setupRenderer();
    void setupInteraction();
    void setInteractorStyle(const std::string & name);

    void updateWindowTitle();
    
    void updateVertexNormals(vtkPolyData * polyData);
    
    void setupAxes(const double bounds[6]);
    static vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
    void setupColorMappingLegend();

    void closeEvent(QCloseEvent * event) override;

private slots:
    /** Updates the RenderConfigWidget to reflect the actors render properties. */
    void on_actorPicked(vtkActor * actor);

private:
    Ui_RenderWidget * m_ui;

    const int m_index;

    // rendered representations of data objects for this view
    QList<RenderedData *> m_renderedData;
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
