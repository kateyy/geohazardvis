#pragma once

#include <memory>

#include <QDockWidget>
#include <QList>

#include <vtkSmartPointer.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkCubeAxesActor;

class Property;
class Input;
class PolyDataInput;
class GridDataInput;

class DataChooser;
enum class DataSelection;
class NormalRepresentation;
class PickingInteractionStyle;
class RenderConfigWidget;
class SelectionHandler;
class Ui_RenderWidget;


class RenderWidget : public QDockWidget
{
    Q_OBJECT

public:
    RenderWidget(
        int index,
        const DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget,
        std::shared_ptr<SelectionHandler> selectionHandler);

    int index() const;

    void addObject(std::shared_ptr<Property> representation);
    void setObject(std::shared_ptr<Property> representation);
    const QList<std::shared_ptr<Property>> & inputs();

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;

    vtkProperty * renderProperty();
    const vtkProperty * renderProperty() const;
    
    PickingInteractionStyle * interactStyle();
    const PickingInteractionStyle * interactStyle() const;

public slots:
    void render();

    void ShowInfo(const QStringList &info);
    
    void uiSelectionChanged(int);
    void updateScalarsForColorMaping(DataSelection dataSelection);
    void updateGradientForColorMapping(const QImage & gradient);
    void applyRenderingConfiguration();

signals:
    void closed();

private:
    void setupRenderer();
    void setupInteraction();

    void show3DInput(std::shared_ptr<PolyDataInput> input);
    void showGridInput(std::shared_ptr<GridDataInput> input);

    void updateWindowTitle();
    
    vtkPolyDataMapper * map3DInputScalars(PolyDataInput & input);
    void updateVertexNormals(vtkPolyData * polyData);
    
    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);

    void closeEvent(QCloseEvent * event) override;

private:
    Ui_RenderWidget * m_ui;

    const int m_index;

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkProperty> m_renderProperty;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    vtkSmartPointer<PickingInteractionStyle> m_interactStyle;

    // visualization and annotation
    NormalRepresentation * m_vertexNormalRepresentation;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    
    // configuration widgets
    const DataChooser & m_dataChooser;
    RenderConfigWidget & m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;
    
    QList<std::shared_ptr<Property>> m_inputRepresentations;
};
