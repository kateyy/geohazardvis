#include "InteractorStyleImage.h"

#include <cmath>

#include <QTextStream>
#include <QStringList>

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCallbackCommand.h>

#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkCellPicker.h>
#include <vtkAbstractMapper3D.h>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkIdTypeArray.h>
#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkTriangle.h>
#include <vtkMath.h>
#include <vtkPolyData.h>

#include <core/vtkhelper.h>
#include <core/Input.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : vtkInteractorStyleImage()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
    , m_selectedCellMapper(vtkSmartPointer<vtkDataSetMapper>::New())
    , m_mouseMoved(false)
{
}

void InteractorStyleImage::OnMouseMove()
{
    vtkInteractorStyleImage::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();

    m_mouseMoved = true;
}

void InteractorStyleImage::OnLeftButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    GrabFocus(EventCallbackCommand);

    StartPan();

    m_mouseMoved = false;
}

void InteractorStyleImage::OnLeftButtonUp()
{
    vtkInteractorStyleImage::OnLeftButtonUp();

    if (!m_mouseMoved)
        highlightPickedCell();
    
    m_mouseMoved = false;
}

void InteractorStyleImage::OnMiddleButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyleImage::OnMiddleButtonUp()
{
    switch (State)
    {
    case VTKIS_DOLLY:
        EndDolly();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnMiddleButtonUp();
    }
}

void InteractorStyleImage::OnRightButtonDown()
{
    OnLeftButtonDown();
}

void InteractorStyleImage::OnRightButtonUp()
{
    OnLeftButtonUp();
}

void InteractorStyleImage::OnChar()
{
    // disable magic keys for now
}

void InteractorStyleImage::setRenderedDataList(const QList<RenderedData *> * renderedData)
{
    assert(renderedData);
    m_renderedData = renderedData;
}

void InteractorStyleImage::highlightPickedCell()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    vtkIdType cellId = m_cellPicker->GetCellId();

    vtkActor * pickedActor = m_cellPicker->GetActor();
    if (!pickedActor)
    {
        highlightCell(-1, nullptr);
        return;
    }

    
    emit actorPicked(pickedActor);

    for (RenderedData * renderedData : *m_renderedData)
    {
        if (renderedData->mainActor() == pickedActor)
            emit cellPicked(renderedData->dataObject(), cellId);
    }
}

void InteractorStyleImage::highlightCell(vtkIdType cellId, DataObject * dataObject)
{
    if (cellId == -1)
    {
        GetDefaultRenderer()->RemoveViewProp(m_selectedCellActor);
        return;
    }

    assert(dataObject);


    VTK_CREATE(vtkIdTypeArray, ids);
    ids->SetNumberOfComponents(1);
    ids->InsertNextValue(cellId);

    VTK_CREATE(vtkSelectionNode, selectionNode);
    selectionNode->SetFieldType(vtkSelectionNode::CELL);
    selectionNode->SetContentType(vtkSelectionNode::INDICES);
    selectionNode->SetSelectionList(ids);

    VTK_CREATE(vtkSelection, selection);
    selection->AddNode(selectionNode);

    VTK_CREATE(vtkExtractSelection, extractSelection);
    extractSelection->SetInputData(0, dataObject->input()->data());
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    m_selectedCellMapper->SetInputConnection(extractSelection->GetOutputPort());

    m_selectedCellActor->SetMapper(m_selectedCellMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);

    GetDefaultRenderer()->AddViewProp(m_selectedCellActor);
    GetDefaultRenderer()->GetRenderWindow()->Render();
}

void InteractorStyleImage::lookAtCell(vtkPolyData * /*polyData*/, vtkIdType /*cellId*/)
{
}

void InteractorStyleImage::sendPointInfo() const
{
    vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();
    if (!mapper)
    {
        emit pointInfoSent(QStringList());
        return;
    }

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    std::string inputname;

    vtkInformation * inputInfo = mapper->GetInformation();

    if (inputInfo->Has(Input::NameKey()))
        inputname = Input::NameKey()->Get(inputInfo);

    double * pos = m_pointPicker->GetPickPosition();

    stream
        << "input file: " << QString::fromStdString(inputname) << endl
        << "selected point: " << endl
        << pos[0] << endl
        << pos[1] << endl
        //<< "value: " << << endl // TODO extract value from picked cell
        << "id: " << m_pointPicker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
