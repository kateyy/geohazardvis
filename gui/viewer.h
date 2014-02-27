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

class QStringList;

class Input;
class Input3D;
class GridDataInput;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();

public slots:
    void ShowInfo(const QStringList &info);

    void on_actionOpen_triggered();

protected:
    Ui_Viewer *m_ui;

    void show3DInput(Input3D & input);
    void showGridInput(GridDataInput & input);

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderer> m_infoRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<vtkRenderWindowInteractor> m_infoInteractor;

    std::list<std::shared_ptr<Input>> m_inputs;

    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;

    void setupRenderer();
    void setupInteraction();

    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkCubeAxesActor * createAxes(vtkRenderer & renderer);
};
