#include "InteractorStyle3D.h"

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
#include <vtkCellData.h>

#include <core/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : Superclass()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
    , m_selectedCellMapper(vtkSmartPointer<vtkDataSetMapper>::New())
    , m_mouseMoved(false)
{
}

void InteractorStyle3D::setRenderedData(QList<RenderedData *> renderedData)
{
    m_actorToRenderedData.clear();
    for (RenderedData * r : renderedData)
        m_actorToRenderedData.insert(r->mainActor(), r);
}

void InteractorStyle3D::OnMouseMove()
{
    Superclass::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());
    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();

    m_mouseMoved = true;
}

void InteractorStyle3D::OnLeftButtonDown()
{
    Superclass::OnLeftButtonDown();

    m_mouseMoved = false;
}

void InteractorStyle3D::OnLeftButtonUp()
{
    Superclass::OnLeftButtonUp();

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

void InteractorStyle3D::OnMouseWheelForward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(true);
}

void InteractorStyle3D::OnMouseWheelBackward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(false);
}

void InteractorStyle3D::OnChar()
{
    // disable most magic keys

    if (this->Interactor->GetKeyCode() == 'l')
        Superclass::OnChar();
}

void InteractorStyle3D::MouseWheelDolly(bool forward)
{
    if (!CurrentRenderer)
        return;

    GrabFocus(EventCallbackCommand);
    StartDolly();

    double factor = MotionFactor * 0.2 * MouseWheelMotionFactor;
    if (!forward)
        factor *= -1;
    factor = std::pow(1.1, factor);

    vtkCamera * camera = CurrentRenderer->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
    }
    else
    {
        camera->Dolly(factor);
        if (AutoAdjustCameraClippingRange)
        {
            CurrentRenderer->ResetCameraClippingRange();
        }
    }

    if (Interactor->GetLightFollowCamera())
    {
        CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

    EndDolly();
    ReleaseFocus();

    Interactor->Render();
}

void InteractorStyle3D::highlightPickedCell()
{
    // assume cells and points picked (in OnMouseMove)

    vtkIdType cellId = m_cellPicker->GetCellId();
    vtkIdType pointId = m_pointPicker->GetPointId();

    if (cellId == -1 && pointId == -1)
    {
        highlightCell(nullptr , - 1);
        return;
    }

    vtkActor * pickedActor = cellId != -1
        ? m_cellPicker->GetActor()
        : m_pointPicker->GetActor();

    RenderedData * renderedData = m_actorToRenderedData.value(pickedActor);
    assert(renderedData);
    
    emit dataPicked(renderedData);

    emit cellPicked(renderedData->dataObject(), cellId);
}

void InteractorStyle3D::highlightCell(DataObject * dataObject, vtkIdType cellId)
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
    vtkAbstractMapper3D * cellMapper = m_cellPicker->GetMapper();
    vtkAbstractMapper3D * pointMapper = m_pointPicker->GetMapper();
    if (!cellMapper && ! pointMapper)    // no object at cursor position
    {
        emit pointInfoSent({});
        return;
    }


    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    vtkInformation * inputInfo = cellMapper
        ? cellMapper->GetInformation()
        : pointMapper->GetInformation();

    QString inputName;
    if (inputInfo->Has(DataObject::NameKey()))
        inputName = DataObject::NameKey()->Get(inputInfo);

    stream
        << "data set: " << inputName << endl;


    vtkCellData * cellData = nullptr; vtkDataArray * centroids = nullptr;
    if (cellMapper && (cellData = m_cellPicker->GetDataSet()->GetCellData()))
        centroids = cellData->GetArray("centroid");

    // for poly data: centroid and scalar information if available
    if (centroids)
    {
        vtkIdType cellId = m_cellPicker->GetCellId();
        double * centroid = centroids->GetTuple(cellId);
        stream
            << "triangle index: " << cellId << endl
            << "x: " << centroid[0] << endl
            << "y: " << centroid[1] << endl
            << "z: " << centroid[2];

        vtkMapper * concreteMapper = vtkMapper::SafeDownCast(cellMapper);
        assert(concreteMapper);
        vtkDataArray * scalars = cellData->GetScalars();
        if (concreteMapper && scalars)
        {
            double value =
                scalars->GetTuple(cellId)[concreteMapper->GetArrayComponent()];
            stream << endl << "scalar value: " << value;
        }
    }
    // for 3D vectors, or poly data fallback
    else
    {
        double* pos = m_pointPicker->GetPickPosition();
        stream
            << "point index: " << m_pointPicker->GetPointId() << endl
            << "x: " << pos[0] << endl
            << "y: " << pos[1] << endl
            << "z: " << pos[2];
    }

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
