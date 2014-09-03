#include "InteractorStyle3D.h"

#include <cmath>

#include <QTextStream>
#include <QStringList>

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

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
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : vtkInteractorStyleTrackballCamera()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
    , m_selectedCellMapper(vtkSmartPointer<vtkDataSetMapper>::New())
    , m_mouseMoved(false)
{
}

void InteractorStyle3D::OnMouseMove()
{
    vtkInteractorStyleTrackballCamera::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();

    m_mouseMoved = true;
}

void InteractorStyle3D::OnLeftButtonDown()
{
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

    m_mouseMoved = false;
}

void InteractorStyle3D::OnLeftButtonUp()
{
    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();

    if (!m_mouseMoved)
        highlightPickedCell();
    
    m_mouseMoved = false;
}

void InteractorStyle3D::OnMiddleButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyle3D::OnMiddleButtonUp()
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

void InteractorStyle3D::OnRightButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartPan();
}

void InteractorStyle3D::OnRightButtonUp()
{
    switch (State)
    {
    case VTKIS_PAN:
        EndPan();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnRightButtonUp();
    }
}

void InteractorStyle3D::OnChar()
{
    // disable magic keys for now
}

void InteractorStyle3D::setRenderedDataList(const QList<RenderedData *> * renderedData)
{
    assert(renderedData);
    m_renderedData = renderedData;
}

void InteractorStyle3D::highlightPickedCell()
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

void InteractorStyle3D::highlightCell(vtkIdType cellId, DataObject * dataObject)
{
    if (cellId == -1)
    {
        GetDefaultRenderer()->RemoveViewProp(m_selectedCellActor);
        return;
    }

    assert(dataObject);


    // extract picked triangle and create highlighting geometry

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

    m_selectedCellMapper->SetInputConnection(extractSelection->GetOutputPort());

    m_selectedCellActor->SetMapper(m_selectedCellMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);

    GetDefaultRenderer()->AddViewProp(m_selectedCellActor);
    GetDefaultRenderer()->GetRenderWindow()->Render();
}

void InteractorStyle3D::lookAtCell(DataObject * dataObject, vtkIdType cellId)
{
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(dataObject->dataSet());

    vtkTriangle * triangle = vtkTriangle::SafeDownCast(polyData->GetCell(cellId));
    assert(triangle);

    VTK_CREATE(vtkPoints, selectedPoints);
    polyData->GetPoints()->GetPoints(triangle->GetPointIds(), selectedPoints);

    // look at center of the object
    const double * objectCenter = polyData->GetCenter();
    GetDefaultRenderer()->GetActiveCamera()->SetFocalPoint(objectCenter);

    // place camera along the normal of the triangle
    double triangleCenter[3];
    vtkTriangle::TriangleCenter(
        selectedPoints->GetPoint(0), selectedPoints->GetPoint(1), selectedPoints->GetPoint(2),
        triangleCenter);

    double triangleNormal[3];
    vtkTriangle::ComputeNormal(polyData->GetPoints(), 0, triangle->GetPointIds()->GetPointer(0), triangleNormal);

    double eyePosition[3];  // current eye position: starting point
    GetDefaultRenderer()->GetActiveCamera()->GetPosition(eyePosition);

    float distance2ObjTri = (float)vtkMath::Distance2BetweenPoints(objectCenter, triangleCenter);
    float distance2EyeObj = (float)vtkMath::Distance2BetweenPoints(objectCenter, eyePosition);
    float distanceEyeTri = std::sqrt(distance2EyeObj - distance2ObjTri);

    vtkMath::MultiplyScalar(triangleNormal, distanceEyeTri);
    double targetEyePosition[3];
    vtkMath::Add(triangleCenter, triangleNormal, targetEyePosition);

    const int NumberOfFlyFrames = 10;

    double pathVector[3];   // distance vector between two succeeding eye positions
    vtkMath::Subtract(targetEyePosition, eyePosition, pathVector);
    vtkMath::MultiplyScalar(pathVector, 1.0 / NumberOfFlyFrames);

    for (int i = 0; i < NumberOfFlyFrames; ++i)
    {
        vtkMath::Add(eyePosition, pathVector, eyePosition);
        GetDefaultRenderer()->GetActiveCamera()->SetPosition(eyePosition);
        GetDefaultRenderer()->ResetCameraClippingRange();
        GetDefaultRenderer()->GetRenderWindow()->Render();
    }
}

void InteractorStyle3D::sendPointInfo() const
{
    vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();
    if (!mapper)    // no object at cursor position
    {
        emit pointInfoSent({});
        return;
    }


    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    double* pos = m_pointPicker->GetPickPosition();

    vtkInformation * inputInfo = mapper->GetInformation();

    std::string inputname;
    if (inputInfo->Has(DataObject::NameKey()))
        inputname = DataObject::NameKey()->Get(inputInfo);

    stream
        << "input file: " << QString::fromStdString(inputname) << endl
        << "selected point: " << endl
        << pos[0] << endl
        << pos[1] << endl
        << pos[2] << endl
        << "id: " << m_pointPicker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
