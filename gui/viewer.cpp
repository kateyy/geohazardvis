#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkProperty.h>


// sources
#include <vtkConeSource.h>
#include <vtkSphereSource.h>
#include <vtkVectorText.h>
// filters
#include <vtkElevationFilter.h>
// mappers
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
// actors
#include <vtkActor2D.h>
// rendering
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
// interaction
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include "pickinginteractionstyle.h"
#include <vtkBoxWidget.h>
#include <vtkPointPicker.h>
// gui/qt
#include <QVTKWidget.h>
#include <QTreeView>


#include "core/datagenerator.h"

class TransformCallback : public vtkCommand
{
public:
    static TransformCallback *New()
    {
        return new TransformCallback;
    }
    virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
        vtkTransform *t = vtkTransform::New();
        vtkBoxWidget *widget = dynamic_cast<vtkBoxWidget*>(caller);
        assert(widget);
        widget->GetTransform(t);
        widget->GetProp3D()->SetUserTransform(t);
        t->Delete();
    }
};

Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();

    loadInputs();

    addInfos();

    addLabels();

    m_mainRenderer->ResetCamera();
}

void Viewer::setupRenderer()
{
    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_infoRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_ui->qvtkInfo->GetRenderWindow()->AddRenderer(m_infoRenderer);
}

void Viewer::setupInteraction()
{
    /** setup interaction */
    vtkSmartPointer<PickingInteractionStyle> interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyle->SetDefaultRenderer(m_mainRenderer);
    interactStyle->setViewer(*this);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void Viewer::loadInputs()
{
    m_volcanoMapper = DataGenerator().generate(5, 3, 0.5f);

    vtkSmartPointer<vtkSphereSource> volcanoCore = vtkSmartPointer<vtkSphereSource>::New();
    volcanoCore->SetRadius(0.6);
    volcanoCore->SetCenter(0, 0.6, 0);

    m_volcanoCoreMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_volcanoCoreMapper->SetInputConnection(volcanoCore->GetOutputPort());

    vtkSmartPointer<vtkActor> volcanoActor = vtkSmartPointer<vtkActor>::New();
    volcanoActor->SetMapper(m_volcanoMapper);
    volcanoActor->GetProperty()->SetRepresentationToWireframe();

    vtkSmartPointer<vtkActor> volcanoCoreActor = vtkSmartPointer<vtkActor>::New();
    volcanoCoreActor->SetMapper(m_volcanoCoreMapper);
    volcanoCoreActor->GetProperty()->SetRepresentationToWireframe();


    /** add actors to the renderer */
    m_mainRenderer->AddActor(volcanoActor);
    m_mainRenderer->AddActor(volcanoCoreActor);
}

void Viewer::addInfos()
{
    vtkSmartPointer<vtkActor> infoActor = vtkSmartPointer<vtkActor>::New();
    infoActor->SetMapper(m_volcanoCoreMapper);

    m_infoRenderer->AddActor(infoActor);
}

void Viewer::addLabels()
{

    /** create text with filter, mapper and actor */
    vtkSmartPointer<vtkVectorText> text = vtkSmartPointer<vtkVectorText>::New();
    text->SetText("vtk-qt-testing");

    vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
    elevation->SetInputConnection(text->GetOutputPort());
    elevation->SetLowPoint(0, 0, 0);
    elevation->SetHighPoint(10, 0, 0);

    vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //textMapper->ImmediateModeRenderingOn();
    textMapper->SetInputConnection(elevation->GetOutputPort());

    vtkSmartPointer<vtkActor> textActor = vtkSmartPointer<vtkActor>::New();
    textActor->SetMapper(textMapper);
    textActor->SetPosition(-6.0, 3.0, 0.0);

    m_mainRenderer->AddActor(textActor);

}

void Viewer::ShowInfo(QString info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItem(info);
}
