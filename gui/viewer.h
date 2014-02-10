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

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();

public slots:
    void ShowInfo(const QStringList &info);

    void on_actionSphere_triggered();
    void on_actionVolcano_triggered();
    void on_actionObservation_triggered();

private:
    Ui_Viewer *m_ui;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderer> m_infoRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<vtkRenderWindowInteractor> m_infoInteractor;

    std::list<std::shared_ptr<Input>> m_inputs;

    QMap<QString, QVector<vtkSmartPointer<vtkProp>>> m_loadedInputs;

    void setCurrentMainInput(const QString & name);

    void setupRenderer();
    void setupInteraction();
    void loadInputs();
    vtkCubeAxesActor * createAxes(double bounds[6], vtkRenderer & renderer);
};
