#pragma once

#include <list>
#include <memory>

#include <QMainWindow>
#include <QMap>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkProp;
class vtkCubeAxesActor;
class PickingInteractionStyle;

class QStringList;

class QVtkTableModel;

class Input;
class PolyDataInput;
class GridDataInput;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();
    virtual ~Viewer() override;

public slots:
    void ShowInfo(const QStringList &info);

    void on_actionOpen_triggered();

    void openFile(QString filename);

    void selectPoint(int index);

protected:
    Ui_Viewer *m_ui;
    QVtkTableModel * m_tableModel;

    void show3DInput(PolyDataInput & input);
    void showGridInput(GridDataInput & input);

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<PickingInteractionStyle> m_interactStyle;

    std::list<std::shared_ptr<Input>> m_inputs;

    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;

    void setupRenderer();
    void setupInteraction();

    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
};
