#pragma once

#include <memory>

#include <QDockWidget>
#include <QList>
#include <QMap>

#include <vtkSmartPointer.h>


class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkPolyData;
class vtkCubeAxesActor;
class vtkActor;

class DataObject;
class RenderedData;
class RenderedPolyData;
class RenderedImageData;

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
        DataChooser & dataChooser,
        RenderConfigWidget & renderConfigWidget,
        std::shared_ptr<SelectionHandler> selectionHandler);

    int index() const;

    void addDataObject(std::shared_ptr<DataObject> dataObject);
    void setDataObject(std::shared_ptr<DataObject> dataObject);

    vtkRenderWindow * renderWindow();
    const vtkRenderWindow * renderWindow() const;
    
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

    void show3DInput(std::shared_ptr<RenderedPolyData> renderedPolyData);
    void showGridInput(std::shared_ptr<RenderedImageData> renderedImageData);

    void updateWindowTitle();
    
    void updateVertexNormals(vtkPolyData * polyData);
    
    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);

    void closeEvent(QCloseEvent * event) override;

private slots:
    /** Updates the RenderConfigWidget to reflect the actors render properties. */
    void on_actorPicked(vtkActor * actor);

private:
    Ui_RenderWidget * m_ui;

    const int m_index;

    // properties to render in this view
    QList<std::shared_ptr<RenderedData>> m_renderedData;
    QMap<vtkActor *, std::shared_ptr<RenderedData>> m_actorToRenderedData;

    // Rendering components
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    vtkSmartPointer<PickingInteractionStyle> m_interactStyle;

    // visualization and annotation
    NormalRepresentation * m_vertexNormalRepresentation;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    
    // configuration widgets
    DataChooser & m_dataChooser;
    RenderConfigWidget & m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;
};
