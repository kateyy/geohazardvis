#include "InteractorStyle3D.h"

#include <cassert>
#include <cmath>

#include <QTextStream>
#include <QStringList>
#include <QTime>
#include <QThread>
#include <QTimer>

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkMath.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolygon.h>
#include <vtkPolygon.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkVector.h>

#include <core/utility/vtkcamerahelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/Picker.h>


namespace
{

/** @return -1 if value < 0, 1 else */
template <typename T> char sgn(const T & value)
{
    return static_cast<char>((T(0) <= value) - (value < T(0)));
}

bool circleLineIntersection(double radius, double P0[2], double P1[2], double intersection1[2], double intersection2[2])
{
    double dx = P1[0] - P0[0];
    double dy = P1[1] - P0[1];
    double dr2 = dx * dx + dy * dy;

    double D = P0[0] * P1[1] + P1[0] + P0[1];

    double incidence = radius * radius *  dr2 - D * D;
    if (incidence < 0)
        return false;

    double sqrtIncidence = std::sqrt(incidence);

    intersection1[0] = (D * dy + sgn(dy) * dx * sqrtIncidence) / dr2;
    intersection1[1] = (-D * dx + std::fabs(dy) * sqrtIncidence) / dr2;

    intersection2[0] = (D * dy - sgn(dy) * dx * sqrtIncidence) / dr2;
    intersection2[1] = (-D * dx - std::fabs(dy) * sqrtIncidence) / dr2;

    return true;
}
}

vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : Superclass()
    , m_picker(std::make_unique<Picker>())
    , m_highlighter(std::make_unique<Highlighter>())
    , m_mouseMoved(false)
{
}

InteractorStyle3D::~InteractorStyle3D() = default;

void InteractorStyle3D::OnMouseMove()
{
    Superclass::OnMouseMove();

    vtkVector2i clickPos;
    GetInteractor()->GetEventPosition(clickPos.GetData());
    FindPokedRenderer(clickPos[0], clickPos[1]);
    auto renderer = GetCurrentRenderer();
    assert(renderer);

    m_picker->pick(clickPos, *renderer);

    emit pointInfoSent(m_picker->pickedObjectInfo());

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
        highlightPickedIndex();

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

void InteractorStyle3D::highlightPickedIndex()
{
    // assume cells and points picked (in OnMouseMove)

    highlightIndex(m_picker->pickedDataObject(), m_picker->pickedIndex());

    if (auto vis = m_picker->pickedVisualizedData())
    {
        emit dataPicked(vis);
        emit indexPicked(&vis->dataObject(), m_picker->pickedIndex());
    }
}

DataObject * InteractorStyle3D::highlightedDataObject() const
{
    return m_highlighter->targetObject();
}

vtkIdType InteractorStyle3D::highlightedIndex() const
{
    return m_highlighter->lastTargetIndex();
}

void InteractorStyle3D::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    assert(index < 0 || dataObject);

    m_highlighter->setRenderer(GetCurrentRenderer());
    m_highlighter->setTarget(
        dataObject,
        index,
        m_picker->pickedIndexType());
}

void InteractorStyle3D::lookAtIndex(DataObject * dataObject, vtkIdType index)
{
    vtkDataSet * dataSet = dataObject->dataSet();
    vtkPolyData * polyData = vtkPolyData::SafeDownCast(dataObject->dataSet());

    vtkCamera & camera = *GetCurrentRenderer()->GetActiveCamera();

    double startingFocalPoint[3], targetFocalPoint[3];
    double startingAzimuth, targetAzimuth;
    double startingElevation, targetElevation;

    camera.GetFocalPoint(startingFocalPoint);
    dataSet->GetCenter(targetFocalPoint);   // look at center of the object
    startingAzimuth = TerrainCamera::getAzimuth(camera);
    startingElevation = TerrainCamera::getVerticalElevation(camera);


    // compute target camera position to find azimuth and elevation for the transition
    double objectCenter[3], startingPosition[3];
    dataSet->GetCenter(objectCenter);
    camera.GetPosition(startingPosition);

    double targetPositionXY[2];

    double objectToEye[3];
    vtkMath::Subtract(startingPosition, objectCenter, objectToEye);
    double viewDistanceXY = vtkMath::Normalize2D(objectToEye);

    vtkCell * cell = dataSet->GetCell(index);
    assert(cell);

    double centroid[3];
    auto cellPointIds = vtkSmartPointer<vtkIdTypeArray>::New();
    cellPointIds->SetArray(cell->GetPointIds()->GetPointer(0), cell->GetNumberOfPoints(), true);
    vtkPolygon::ComputeCentroid(cellPointIds, polyData->GetPoints(), centroid);
    double selectionCenterXY[2] = { centroid[0], centroid[1] };

    double cellNormal[3];
    vtkPolygon::ComputeNormal(cell->GetPoints(), cellNormal);

    double norm_objectToSelectionXY[2];
    norm_objectToSelectionXY[0] = selectionCenterXY[0] - objectCenter[0];
    norm_objectToSelectionXY[1] = selectionCenterXY[1] - objectCenter[1];
    double selectionRadiusXY = vtkMath::Normalize2D(norm_objectToSelectionXY);

    // make sure to move outside of the selection
    if (viewDistanceXY < selectionRadiusXY)
        viewDistanceXY = selectionRadiusXY * 1.5;


    // choose nearest viewpoint for flat surfaces
    const double flat_threshold = 30;
    double inclination = std::acos(cellNormal[2]) * 180.0 / vtkMath::Pi();
    if (inclination < flat_threshold)
    {
        targetPositionXY[0] = objectCenter[0] + viewDistanceXY * norm_objectToSelectionXY[0];
        targetPositionXY[1] = objectCenter[1] + viewDistanceXY * norm_objectToSelectionXY[1];
    }
    // or use the hill's normal
    else
    {
        double cellNormalXY[2] = { cellNormal[0], cellNormal[1] };

        double l;
        if ((l = std::sqrt((cellNormalXY[0] * cellNormalXY[0] + cellNormalXY[1] * cellNormalXY[1]))) != 0.0)
        {
            cellNormalXY[0] /= l;
            cellNormalXY[1] /= l;
        }

        // get a point in front of the selected cell
        double selectionFrontXY[2] = { selectionCenterXY[0] + cellNormalXY[0], selectionCenterXY[1] + cellNormalXY[1] };

        double intersections[4];
        // our focal point (center of view circle) is the object center
        // so assume a circle center of (0,0) in the calculations
        bool intersects =
            circleLineIntersection(viewDistanceXY, selectionCenterXY, selectionFrontXY, &intersections[0], &intersections[2]);

        // ignore for now
        if (!intersects)
        {
            targetPositionXY[0] = startingPosition[0];
            targetPositionXY[1] = startingPosition[1];
        }
        else
        {
            bool towardsPositive = selectionFrontXY[0] > selectionCenterXY[0] || (selectionFrontXY[0] == selectionCenterXY[0] && selectionFrontXY[1] >= selectionCenterXY[1]);
            int intersectionIndex;
            if (towardsPositive == (intersections[0] > intersections[2] || (intersections[0] == intersections[2] && intersections[1] >= intersections[3])))
                intersectionIndex = 0;
            else
                intersectionIndex = 2;
            targetPositionXY[0] = intersections[intersectionIndex];
            targetPositionXY[1] = intersections[intersectionIndex + 1];
        }
    }

    auto targetCamera = vtkSmartPointer<vtkCamera>::New();
    targetCamera->SetPosition(targetPositionXY[0], targetPositionXY[1], startingPosition[2]);
    targetCamera->SetFocalPoint(targetFocalPoint);
    targetCamera->SetViewUp(camera.GetViewUp());

    targetAzimuth = TerrainCamera::getAzimuth(*targetCamera);
    targetElevation = TerrainCamera::getVerticalElevation(*targetCamera);


    const double flyTimeSec = 0.5;
    const double flyDeadlineSec = 1;

    const int NumberOfFlyFrames = 15;
    double stepFactor = 1.0 / (NumberOfFlyFrames + 1);


    // transition of position, focal point, azimuth and elevation

    double stepFocalPoint[3], stepPosition[3], stepAzimuth, stepElevation;

    vtkMath::Subtract(targetFocalPoint, startingFocalPoint, stepFocalPoint);
    vtkMath::MultiplyScalar(stepFocalPoint, stepFactor);
    vtkMath::Subtract(targetCamera->GetPosition(), startingPosition, stepPosition);
    vtkMath::MultiplyScalar(stepPosition, stepFactor);
    stepAzimuth = (targetAzimuth - startingAzimuth) * stepFactor;
    stepElevation = (targetElevation - startingElevation) * stepFactor;


    double intermediateFocal[3]{ startingFocalPoint[0], startingFocalPoint[1], startingFocalPoint[2] };
    double intermediatePosition[3]{ startingPosition[0], startingPosition[1], startingPosition[2] };
    double intermediateAzimuth = startingAzimuth;
    double intermediateElevation = startingElevation;

    QTime startingTime = QTime::currentTime();
    QTime deadline = startingTime.addSecs(flyDeadlineSec);
    long sleepMSec = long(flyTimeSec * 1000.0 * stepFactor);

    QTime renderTime;
    int i = 0;
    for (; i < NumberOfFlyFrames; ++i)
    {
        renderTime.start();

        vtkMath::Add(intermediateFocal, stepFocalPoint, intermediateFocal);
        vtkMath::Add(intermediatePosition, stepPosition, intermediatePosition);
        intermediateAzimuth += stepAzimuth;
        intermediateElevation += stepElevation;

        camera.SetFocalPoint(intermediateFocal);
        camera.SetPosition(intermediatePosition);
        TerrainCamera::setAzimuth(camera, intermediateAzimuth);
        TerrainCamera::setVerticalElevation(camera, intermediateElevation);

        GetCurrentRenderer()->ResetCameraClippingRange();
        GetCurrentRenderer()->GetRenderWindow()->Render();

        if (QTime::currentTime() > deadline)
            break;

        QThread::msleep((unsigned long)std::max(0l, sleepMSec - renderTime.elapsed()));
    }

    // jump to target if flight rendering was too slow
    if (i < NumberOfFlyFrames)
    {
        camera.SetFocalPoint(targetFocalPoint);
        camera.SetPosition(targetCamera->GetPosition());
        TerrainCamera::setAzimuth(camera, targetAzimuth);
        TerrainCamera::setVerticalElevation(camera, targetElevation);
    }
}

void InteractorStyle3D::flashHightlightedCell(unsigned int milliseconds)
{
    m_highlighter->setFlashTimeMilliseconds(milliseconds);
    m_highlighter->flashTargets();
}
