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
    GetDefaultRenderer()->RemoveViewProp(m_selectedCellActor);
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

namespace
{

/** @return -1 if value < 0, 1 else */
template <typename T> char sgn(const T & value)
{
    return (T(0) <= value) - (value < T(0));
}

bool circleLineIntersection(double radius, double P0[2], double P1[2], double intersection[2])
{
    double dx = P1[0] - P0[0];
    double dy = P1[1] - P0[1];
    double dr2 = dx * dx + dy * dy;

    double D = P0[0] * P1[1] + P1[0] + P0[1];

    double incidence = radius * radius *  dr2 - D * D;
    if (incidence < 0)
        return false;

    double sqrtIncidence = std::sqrt(incidence);

    // switch +/- for second intersection
    intersection[0] = (D * dy + sgn(dy) * dx * sqrtIncidence) / dr2;
    intersection[1] = (-D * dx + std::fabs(dy) * sqrtIncidence) / dr2;

    return true;
}
}

void InteractorStyle3D::lookAtCell(DataObject * dataObject, vtkIdType cellId)
{
    vtkDataSet * dataSet = dataObject->dataSet();
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(dataObject->dataSet());

    vtkCamera * camera = GetDefaultRenderer()->GetActiveCamera();
    const double * objectCenter = dataSet->GetCenter();
    const double * eyePosition = camera->GetPosition();

    // look at center of the object
    const double * targetFocalPoint(objectCenter);
    double targetPositionXY[2];


    double objectToEye[3];
    vtkMath::Subtract(eyePosition, objectCenter, objectToEye);
    double viewDistanceXY = vtkMath::Normalize2D(objectToEye);

    vtkDataArray * centroids = dataSet->GetCellData()->GetArray("centroid");
    assert(centroids && centroids->GetNumberOfComponents() == 3);
    double selectionCenterXY[2] = { centroids->GetTuple(cellId)[0], centroids->GetTuple(cellId)[1] };

    vtkTriangle * triangle = vtkTriangle::SafeDownCast(dataSet->GetCell(cellId));
    assert(triangle);
    double triangleNormal[3];
    vtkTriangle::ComputeNormal(polyData->GetPoints(), 0, triangle->GetPointIds()->GetPointer(0), triangleNormal);

    double norm_objectToSelectionXY[3];
    vtkMath::Subtract(selectionCenterXY, objectCenter, norm_objectToSelectionXY);
    double selectionRadiusXY = vtkMath::Normalize2D(norm_objectToSelectionXY);

    // make sure to move outside of the selection
    if (viewDistanceXY < selectionRadiusXY)
        viewDistanceXY = selectionRadiusXY * 1.5;


    // choose nearest viewpoint for flat surfaces
    const double flat_threshold = 30;
    double inclination = std::acos(triangleNormal[2]) * 180.0 / vtkMath::Pi();
    if (inclination < flat_threshold)
    {
        targetPositionXY[0] = norm_objectToSelectionXY[0];
        targetPositionXY[1] = norm_objectToSelectionXY[1];
        vtkMath::MultiplyScalar2D(targetPositionXY, viewDistanceXY);
    }

    // or use the hill's normal
    else
    {
        double triangleNormalXY[2] = { triangleNormal[0], triangleNormal[1] };

        double l;
        if ((l = std::sqrt((triangleNormalXY[0] * triangleNormalXY[0] + triangleNormalXY[1] * triangleNormalXY[1]))) != 0.0)
        {
            triangleNormalXY[0] /= l;
            triangleNormalXY[1] /= l;
        }

        // get a point in front of the selected cell
        double selectionFrontXY[2] = { selectionCenterXY[0] + triangleNormalXY[0], selectionCenterXY[1] + triangleNormalXY[1] };

        double intersection[2];
        // our focal point (center of view circle) is the object center
        // so assume a circle center of (0,0) in the calculations
        bool intersects =
            circleLineIntersection(viewDistanceXY, selectionCenterXY, selectionFrontXY, intersection);
        
        // ignore for now
        if (!intersects)
        {
            targetPositionXY[0] = eyePosition[0];
            targetPositionXY[1] = eyePosition[1];
        }

        targetPositionXY[0] = intersection[0];
        targetPositionXY[1] = intersection[1];
    }


    const int NumberOfFlyFrames = 10;

    double pathVectorFocal[3];
    const double * currentFocal = camera->GetFocalPoint();
    vtkMath::Subtract(targetFocalPoint, currentFocal, pathVectorFocal);
    vtkMath::MultiplyScalar(pathVectorFocal, 1.0 / (NumberOfFlyFrames + 1));

    double pathVectorPos[3];   // distance vector between two succeeding eye positions
    pathVectorPos[0] = targetPositionXY[0] - eyePosition[0];
    pathVectorPos[1] = targetPositionXY[1] - eyePosition[1];
    pathVectorPos[2] = 0;
    vtkMath::MultiplyScalar(pathVectorPos, 1.0 / (NumberOfFlyFrames + 1));

    double intermediateFocal[3]{ currentFocal[0], currentFocal[1], currentFocal[2] };
    double intermediateEye[3]{ eyePosition[0], eyePosition[1], eyePosition[2] };

    for (int i = 0; i < NumberOfFlyFrames; ++i)
    {
        vtkMath::Add(intermediateFocal, pathVectorFocal, intermediateFocal);
        vtkMath::Add(intermediateEye, pathVectorPos, intermediateEye);
        camera->SetFocalPoint(intermediateFocal);
        camera->SetPosition(intermediateEye);
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
