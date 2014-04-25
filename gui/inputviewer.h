#pragma once

#include <memory>
#include <array>

#include <QWidget>
#include <QMap>
#include <QVector>

#include <vtkSmartPointer.h>

class Ui_InputViewer;

class QVtkTableModel;

class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkCubeAxesActor;
class vtkProp;
class PickingInteractionStyle;

class SelectionHandler;
class Input;
class PolyDataInput;
class GridDataInput;

class InputViewer : public QWidget
{
    Q_OBJECT

public:
    InputViewer(QWidget * parent = nullptr);
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

    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);

protected:
    Ui_InputViewer * m_ui;
    QVtkTableModel * m_tableModel;
    std::shared_ptr<SelectionHandler> m_selectionHandler;
    QVector<QImage> m_scalarToColorGradients;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<PickingInteractionStyle> m_interactStyle;
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;

    std::list<std::shared_ptr<Input>> m_inputs;
    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;
};
