#pragma once

#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>

#include "AbstractDataView.h"

#include <core/scalar_mapping/ScalarToColorMapping.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkPolyData;
class vtkCubeAxesActor;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkActor;
class vtkLightKit;

class DataObject;
class RenderedData;

class ScalarMappingChooser;
class VectorMappingChooser;
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
        ScalarMappingChooser & scalarMappingChooser,
        VectorMappingChooser & vectorMappingChooser,
        RenderConfigWidget & renderConfigWidget,
        QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~RenderView() override;

    bool isTable() const override;
    bool isRenderer() const override;

    QString friendlyName() const override;

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

    vtkRenderer * renderer();
    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;
    
    IPickingInteractorStyle * interactorStyle();
    const IPickingInteractorStyle * interactorStyle() const;

    vtkLightKit * lightKit();

    bool contains3dData() const;

public slots:
    void render();

    void ShowInfo(const QStringList &info);

protected:
    QWidget * contentWidget() override;

private:
    void setupRenderer();
    void setupInteraction();
    void setInteractorStyle(const std::string & name);

    // data handling

    RenderedData * addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);
    /** reduce list to compatible objects, show GUI warning if needed */
    QStringList checkCompatibleObjects(QList<DataObject *> & dataObjects);

    // remove some data objects from internal lists
    void removeFromInternalLists(QList<DataObject *> dataObjects = {});
    void clearInternalLists();

    // GUI / rendering tools
    
    void updateVertexNormals(vtkPolyData * polyData);
    
    void updateAxes();
    static vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
    void setupColorMappingLegend();

    void updateInteractionType();

    void warnIncompatibleObjects(QStringList incompatibleObjects);

private slots:
    /** Updates the RenderConfigWidget to reflect the data's render properties. */
    void updateGuiForData(RenderedData * renderedData);

private:
    Ui_RenderView * m_ui;

    // rendered representations of data objects for this view
    QList<RenderedData *> m_renderedData;
    // objects that were loaded to the GPU but are currently not rendered 
    QList<RenderedData *> m_renderedDataCache;
    QMap<DataObject *, RenderedData *> m_dataObjectToRendered;

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<PickingInteractorStyleSwitch> m_interactorStyle;
    vtkSmartPointer<vtkLightKit> m_lightKit;

    // visualization and annotation
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkScalarBarActor * m_colorMappingLegend;
    vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
    
    // configuration widgets
    ScalarMappingChooser & m_scalarMappingChooser;
    VectorMappingChooser & m_vectorMappingChooser;
    ScalarToColorMapping m_scalarMapping;
    RenderConfigWidget & m_renderConfigWidget;

    bool m_contains3DData;
};
