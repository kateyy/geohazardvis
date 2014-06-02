#pragma once

#include <memory>

#include <QDockWidget>

#include <vtkSmartPointer.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkProperty;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkCubeAxesActor;

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
        const DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget,
        std::shared_ptr<SelectionHandler> selectionHandler);

    void show3DInput(std::shared_ptr<PolyDataInput> input);
    void showGridInput(std::shared_ptr<GridDataInput> input);

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

private:
    void setupRenderer();
    void setupInteraction();
    
    vtkPolyDataMapper * map3DInputScalars(PolyDataInput & input);
    void updateVertexNormals(vtkPolyData * polyData);
    
    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);

private:
    Ui_RenderWidget * m_ui;

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
    
    std::list<std::shared_ptr<Input>> m_inputs;
};
