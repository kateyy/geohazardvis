#pragma once

#include <memory>
#include <array>

#include <QMainWindow>
#include <QMap>
#include <QVector>

#include <vtkSmartPointer.h>

class Ui_InputViewer;
class QTableView;

class QVtkTableModel;

class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkCubeAxesActor;
class vtkProp;
class vtkPolyData;
class PickingInteractionStyle;

class SelectionHandler;
class Input;
class PolyDataInput;
class GridDataInput;
class RenderConfigWidget;

class InputViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit InputViewer(QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
    ~InputViewer() override;

    bool isEmpty() const;

signals:
    void dockingRequested();

public slots:
    void ShowInfo(const QStringList &info);
    void openFile(QString filename);

protected slots:
    void uiSelectionChanged(int);
    void updateScalarToColorMapping();

protected:
    void setupRenderer();
    void setupInteraction();
    void loadGradientImages();

    void showEvent(QShowEvent * event) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

    void show3DInput(PolyDataInput & input);
    void showGridInput(GridDataInput & input);

    vtkPolyDataMapper * map3DInputScalars(PolyDataInput & input);
    void showVertexNormals(vtkPolyData * polyData);

    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);

protected:
    Ui_InputViewer * m_ui;
    QTableView * m_tableView;
    QVtkTableModel * m_tableModel;
    RenderConfigWidget * m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;
    QVector<QImage> m_scalarToColorGradients;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<PickingInteractionStyle> m_interactStyle;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;

    std::list<std::shared_ptr<Input>> m_inputs;
    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;
};
