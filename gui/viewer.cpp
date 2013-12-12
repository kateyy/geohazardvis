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
#include <vtkBoxWidget.h>
// gui
#include <vtkQtTableView.h>
#include <vtkDataObjectToTable.h>
#include <QVTKWidget.h>

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

    addLabels();

    m_tableView = vtkSmartPointer<vtkQtTableView>::New();
    m_ui->infoSplitter->addWidget(m_tableView->GetWidget());

    /** pass infos to qt gui */
    //vtkSmartPointer<vtkDataObjectToTable> toTable = vtkSmartPointer<vtkDataObjectToTable>::New();
    //toTable->SetInputConnection(elevation->GetOutputPort());
    //toTable->SetFieldType(vtkDataObjectToTable::POINT_DATA);

    /*m_tableView->SetRepresentationFromInputConnection(toTable->GetOutputPort());
    m_tableView->Update();*/
}

Viewer::~Viewer()
{

}

void Viewer::setupRenderer()
{
    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    //m_renderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_infoRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_ui->qvtkInfo->GetRenderWindow()->AddRenderer(m_infoRenderer);
}

void Viewer::setupInteraction()
{
    /** setup interaction */
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> interactStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interactor->SetInteractorStyle(interactStyle);
    m_interactor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_interactor->Initialize();

    //vtkSmartPointer<vtkBoxWidget> boxWidget = vtkSmartPointer<vtkBoxWidget>::New();
    //boxWidget->SetInteractor(m_interactor);
    //boxWidget->SetPlaceFactor(1.25);

    //boxWidget->SetProp3D(cone1Actor);
    //boxWidget->PlaceWidget();
    //vtkSmartPointer<TransformCallback> callback = vtkSmartPointer<TransformCallback>::New();
    //boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);


    //boxWidget->On();
}

void Viewer::loadInputs()
{
    /** create "volcano" with mapper and actor */
    vtkSmartPointer<vtkConeSource> volcano = vtkSmartPointer<vtkConeSource>::New();
    volcano->SetResolution(50);
    volcano->SetDirection(0, 1, 0);
    volcano->SetHeight(4.0);
    volcano->SetRadius(5.0);

    vtkSmartPointer<vtkSphereSource> volcanoCore = vtkSmartPointer<vtkSphereSource>::New();
    volcanoCore->SetRadius(0.6);
    volcanoCore->SetCenter(0, 0.6, 0);

    vtkSmartPointer<vtkPolyDataMapper> volcanoMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    volcanoMapper->SetInputConnection(volcano->GetOutputPort());
    vtkSmartPointer<vtkPolyDataMapper> volcanoCoreMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    volcanoCoreMapper->SetInputConnection(volcanoCore->GetOutputPort());

    vtkSmartPointer<vtkActor> volcanoActor = vtkSmartPointer<vtkActor>::New();
    volcanoActor->SetMapper(volcanoMapper);
    volcanoActor->GetProperty()->SetRepresentationToWireframe();
    vtkSmartPointer<vtkActor> volcanoCoreActor = vtkSmartPointer<vtkActor>::New();
    volcanoCoreActor->SetMapper(volcanoCoreMapper);
    volcanoCoreActor->GetProperty()->SetRepresentationToWireframe();


    /** add actors to the renderer */
    m_mainRenderer->AddActor(volcanoActor);
    m_mainRenderer->AddActor(volcanoCoreActor);
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
