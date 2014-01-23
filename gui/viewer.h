#pragma once

#include <list>
#include <memory>

#include <QMainWindow>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkQtTableView;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkActor;
class vtkPolyDataMapper;

class QStringList;

class vtkPointPicker;
struct Input;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();

public slots:
    void ShowInfo(const QStringList &info);

private:
    Ui_Viewer *m_ui;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderer> m_infoRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<vtkRenderWindowInteractor> m_infoInteractor;

    std::list<std::shared_ptr<Input>> m_inputs;

    void setupRenderer();
    void setupInteraction();
    void loadInputs();
    void setupAxis(const Input & input, vtkRenderer & renderer);
};
