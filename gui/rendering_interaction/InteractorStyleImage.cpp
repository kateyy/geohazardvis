#include "InteractorStyleImage.h"

#include <cmath>

#include <QTextStream>
#include <QStringList>

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkCallbackCommand.h>

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkSelectionNode.h>
#include <vtkSelection.h>
#include <vtkExtractSelection.h>

#include <vtkIdTypeArray.h>
#include <vtkCellData.h>
#include <vtkImageData.h>

#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkTexture.h>
#include <vtkProperty.h>

#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : vtkInteractorStyleImage()
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
    , m_mouseMoved(false)
{
}

void InteractorStyleImage::OnMouseMove()
{
    vtkInteractorStyleImage::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
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

void InteractorStyleImage::setRenderedData(QList<RenderedData *> renderedData)
{
    GetDefaultRenderer()->RemoveViewProp(m_selectedCellActor);
    m_actorToRenderedData.clear();

    for (RenderedData * r : renderedData)
    {
        vtkCollectionSimpleIterator it;
        r->viewProps()->InitTraversal(it);
        while (vtkProp * prop = r->viewProps()->GetNextProp(it))
        {
            if (prop->GetPickable())
                m_actorToRenderedData.insert(prop, r);
        }
    }
}

void InteractorStyleImage::highlightPickedCell()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    vtkIdType cellId = m_cellPicker->GetCellId();

    vtkActor * pickedActor = m_cellPicker->GetActor();
    if (!pickedActor)
    {
        highlightCell(nullptr, -1);
        return;
    }

    RenderedData * renderedData = m_actorToRenderedData.value(pickedActor);
    assert(renderedData);
    
    emit dataPicked(renderedData);

    emit cellPicked(renderedData->dataObject(), cellId);
}

void InteractorStyleImage::highlightCell(DataObject * dataObject, vtkIdType cellId)
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
    extractSelection->SetInputData(0, dataObject->dataSet());
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    VTK_CREATE(vtkDataSetMapper, selectedCellMapper);
    selectedCellMapper->SetInputConnection(extractSelection->GetOutputPort());

    m_selectedCellActor->SetMapper(selectedCellMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);
    m_selectedCellActor->PickableOff();

    GetDefaultRenderer()->AddViewProp(m_selectedCellActor);
    GetDefaultRenderer()->GetRenderWindow()->Render();
}

void InteractorStyleImage::lookAtCell(DataObject * /*polyData*/, vtkIdType /*cellId*/)
{
}

void InteractorStyleImage::sendPointInfo() const
{
    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    do
    {
        vtkAbstractMapper3D * mapper = m_cellPicker->GetMapper();
        if (!mapper)
            break;


        std::string inputname;

        vtkInformation * inputInfo = mapper->GetInformation();
        if (inputInfo->Has(DataObject::NameKey()))
            inputname = DataObject::NameKey()->Get(inputInfo);

        double * pos = m_cellPicker->GetPickPosition();

        vtkTexture * texture = m_cellPicker->GetActor()->GetTexture();
        if (!texture)
            break;

        vtkImageData * image = texture->GetInput();
        if (!image)
            break;
        vtkDataArray * data = image->GetCellData()->GetScalars();
        if (!data)
            break;

        vtkIdType cellId = m_cellPicker->GetCellId();
        double value = data->GetTuple(cellId)[0];

        stream
            << "input file: " << QString::fromStdString(inputname) << endl
            << "selected cell: " << endl
            << "value: " << value << endl
            << "row: " << pos[0] << endl
            << "column: " << pos[1] << endl
            << "id: " << cellId;
    } while (false);

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
