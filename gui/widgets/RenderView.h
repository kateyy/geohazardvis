#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include "AbstractDataView.h"

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
class PickingInteractorStyleSwitch;
class IPickingInteractorStyle;
class RenderConfigWidget;
class Ui_RenderView;


class RenderView : public AbstractDataView
{
    Q_OBJECT

public:
    RenderView(
        int index,
        DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget,
        QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

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

protected:
    QWidget * contentWidget() override;

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

private slots:
    /** Updates the RenderConfigWidget to reflect the actors render properties. */
    void updateGuiForActor(vtkActor * actor);

private:
    Ui_RenderView * m_ui;

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