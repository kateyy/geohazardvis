#pragma once

#include <list>

#include <QMainWindow>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkQtTableView;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkActor;
class vtkPolyDataMapper;

class vtkPointPicker;

class QStringList;

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

    struct Input {
        vtkSmartPointer<vtkPolyDataMapper> inputDataMapper;
        double color[3];

        vtkSmartPointer<vtkActor> createActor();
        void setColor(double r, double g, double b);
    };

    std::list<Input> m_inputs;

    void setupRenderer();
    void setupInteraction();
    void loadInputs();
};
