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
class vtkQtTableView;
class vtkObject;
class vtkCommand;

class QStringList;

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

    void selectionChanged(vtkObject* caller, unsigned long vtk_event, void* clientData, void* callData, vtkCommand* command);

protected:
    Ui_Viewer *m_ui;

    void show3DInput(PolyDataInput & input);
    void showGridInput(GridDataInput & input);

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;

    vtkSmartPointer<vtkQtTableView> m_tableView;

    std::list<std::shared_ptr<Input>> m_inputs;

    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;

    void setupRenderer();
    void setupInteraction();

    void setupAxes(const double bounds[6]);
    vtkSmartPointer<vtkCubeAxesActor> m_axesActor;
    vtkSmartPointer<vtkCubeAxesActor> createAxes(vtkRenderer & renderer);
};
