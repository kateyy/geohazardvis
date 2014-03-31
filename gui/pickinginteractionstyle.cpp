#include "pickinginteractionstyle.h"

#include <cmath>

#include <QDebug>

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
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkFloatArray.h>
#include <vtkTriangle.h>
#include <vtkMath.h>
#include <vtkPolyData.h>

#include <QTextStream>
#include <QStringList>
#include <QItemSelectionModel>

#include "core/vtkhelper.h"
#include "core/input.h"


vtkStandardNewMacro(PickingInteractionStyle);

PickingInteractionStyle::PickingInteractionStyle()
: vtkInteractorStyleTrackballCamera()
, m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
, m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
, m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
, m_selectedCellMapper(vtkSmartPointer<vtkDataSetMapper>::New())
, m_mouseMoved(false)
{
}

void PickingInteractionStyle::OnMouseMove()
{
    pickPoint();
    vtkInteractorStyleTrackballCamera::OnMouseMove();
    m_mouseMoved = true;
}

void PickingInteractionStyle::OnLeftButtonDown()
{
    m_mouseMoved = false;
    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}

void PickingInteractionStyle::OnLeftButtonUp()
{
    if (!m_mouseMoved) {
        pickCell();
    }
    m_mouseMoved = false;

    vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}

void PickingInteractionStyle::pickPoint()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    // picking in the input geometry
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();
}

void PickingInteractionStyle::pickCell()
{
    int* clickPos = GetInteractor()->GetEventPosition();

    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    vtkIdType cellId = m_cellPicker->GetCellId();

    if (cellId != -1)
        emit cellPicked(m_cellPicker->GetDataSet(), cellId);
}

void PickingInteractionStyle::highlightCell(vtkIdType cellId, vtkDataObject * dataObject)
{
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
    extractSelection->SetInputData(0, dataObject);
    extractSelection->SetInputData(1, selection);
    extractSelection->Update();

    VTK_CREATE(vtkUnstructuredGrid, selected);
    selected->ShallowCopy(extractSelection->GetOutput());

    m_selectedCellMapper->SetInputData(selected);

    m_selectedCellActor->SetMapper(m_selectedCellMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);

    GetDefaultRenderer()->AddActor(m_selectedCellActor);

    vtkPolyData * polyData = vtkPolyData::SafeDownCast(dataObject);

    if (polyData)
        lookAtCell(polyData, cellId);
}

void PickingInteractionStyle::lookAtCell(vtkPolyData * polyData, vtkIdType cellId)
{
    // not implemented for grid input
    vtkTriangle * triangle = vtkTriangle::SafeDownCast(polyData->GetCell(cellId));
    if (triangle == nullptr)
        return;

    VTK_CREATE(vtkPoints, selectedPoints);
    polyData->GetPoints()->GetPoints(triangle->GetPointIds(), selectedPoints);


    // look at center of the object
    const double * bounds = polyData->GetBounds();
    const double * objectCenter = polyData->GetCenter();
    GetDefaultRenderer()->GetActiveCamera()->SetFocalPoint(objectCenter);

    // place camera along the normal of the triangle
    double triangleCenter[3];
    vtkTriangle::TriangleCenter(
        selectedPoints->GetPoint(0), selectedPoints->GetPoint(1), selectedPoints->GetPoint(2),
        triangleCenter);

    double triangleNormal[3];
    vtkTriangle::ComputeNormal(polyData->GetPoints(), 0, triangle->GetPointIds()->GetPointer(0), triangleNormal);
    //float scale = 2 * std::max(bounds[1] - bounds[0], std::max(bounds[3] - bounds[2], bounds[6] - bounds[5]));


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
    vtkMath::MultiplyScalar(pathVector, 1.0/NumberOfFlyFrames);

    for (int i = 0; i < NumberOfFlyFrames; ++i)
    {
        vtkMath::Add(eyePosition, pathVector, eyePosition);
        //GetDefaultRenderer()->GetActiveCamera()->OrthogonalizeViewUp();
        GetDefaultRenderer()->GetActiveCamera()->SetPosition(eyePosition);
        GetDefaultRenderer()->ResetCameraClippingRange();
        GetDefaultRenderer()->GetRenderWindow()->Render();
    }
}

void PickingInteractionStyle::sendPointInfo() const
{
    double* pos = m_pointPicker->GetPickPosition();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    std::string inputname;

    vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();

    if (!mapper) {
        emit pointInfoSent(QStringList());
        return;
    }

    vtkInformation * inputInfo = mapper->GetInformation();

    if (inputInfo->Has(Input::NameKey()))
        inputname = Input::NameKey()->Get(inputInfo);

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
