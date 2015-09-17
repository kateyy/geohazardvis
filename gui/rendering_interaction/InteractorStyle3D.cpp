#include "InteractorStyle3D.h"

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

#include <core/utility/vtkcamerahelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>


vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : Superclass()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_cellPicker(vtkSmartPointer<vtkCellPicker>::New())
    , m_selectedCellActor(vtkSmartPointer<vtkActor>::New())
    , m_selectedCellData(vtkSmartPointer<vtkPolyData>::New())
    , m_currentlyHighlighted(nullptr, -1)
    , m_highlightFlashTimer(nullptr)
    , m_highlightFlashTime(new QTime())
    , m_mouseMoved(false)
{
    auto selectionMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    selectionMapper->SetInputData(m_selectedCellData);
    m_selectedCellActor->SetMapper(selectionMapper);
    m_selectedCellActor->GetProperty()->EdgeVisibilityOn();
    m_selectedCellActor->GetProperty()->SetEdgeColor(1, 0, 0);
    m_selectedCellActor->GetProperty()->SetLineWidth(3);
    m_selectedCellActor->PickableOff();
}

InteractorStyle3D::~InteractorStyle3D() = default;

void InteractorStyle3D::setRenderedData(const QList<RenderedData *> & renderedData)
{
    m_actorToRenderedData.clear();
    if (auto renderer = GetCurrentRenderer())
        renderer->RemoveViewProp(m_selectedCellActor);
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

void InteractorStyle3D::OnMouseMove()
{
    Superclass::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
    FindPokedRenderer(clickPos[0], clickPos[1]);

    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetCurrentRenderer());
    m_cellPicker->Pick(clickPos[0], clickPos[1], 0, GetCurrentRenderer());

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

    vtkIdType cellId = m_cellPicker->GetCellId();
    vtkIdType pointId = m_pointPicker->GetPointId();

    if (cellId == -1 && pointId == -1)
    {
        highlightIndex(nullptr , - 1);
        return;
    }

    vtkActor * pickedActor = cellId != -1
        ? m_cellPicker->GetActor()
        : m_pointPicker->GetActor();

    RenderedData * renderedData = m_actorToRenderedData.value(pickedActor);
    if (renderedData)
    {
        vtkIdType index = cellId >= 0 ? cellId : pointId;
        highlightIndex(&renderedData->dataObject(), index);

        emit dataPicked(renderedData);

        emit indexPicked(&renderedData->dataObject(), index);
    }
}

DataObject * InteractorStyle3D::highlightedDataObject() const
{
    return m_currentlyHighlighted.first;
}

vtkIdType InteractorStyle3D::highlightedIndex() const
{
    return m_currentlyHighlighted.second;
}

void InteractorStyle3D::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    if (index == -1)
    {
        if (m_currentlyHighlighted.second < 0)
            return;

        GetCurrentRenderer()->RemoveViewProp(m_selectedCellActor);
        GetCurrentRenderer()->GetRenderWindow()->Render();
        m_currentlyHighlighted = { nullptr, -1 };
        return;
    }

    assert(dataObject);

    // already highlighting this cell, so just flash again
    if (m_currentlyHighlighted == QPair<DataObject *, vtkIdType>(dataObject, index))
    {
        flashHightlightedCell();
        return;
    }

    // extract picked triangle and create highlighting geometry
    // create two shifted polygons to work around occlusion

    vtkCell * selection = dataObject->dataSet()->GetCell(index);

    // this is probably a glyph or the like; we don't have an implementation for that in the moment
    if (selection->GetCellType() == VTK_VOXEL)
        return;

    vtkIdType numberOfPoints = selection->GetNumberOfPoints();

    double cellNormal[3];
    vtkPolygon::ComputeNormal(selection->GetPoints(), cellNormal);
    vtkMath::MultiplyScalar(cellNormal, 0.001);

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(numberOfPoints * 2);
    std::vector<vtkIdType> front, back;
    auto polys = vtkSmartPointer<vtkCellArray>::New();
    for (vtkIdType i = 0; i < numberOfPoints; ++i)
    {
        double original[3], shifted[3];
        selection->GetPoints()->GetPoint(i, original);

        vtkMath::Add(original, cellNormal, shifted);
        points->InsertPoint(i, shifted);
        front.push_back(i);

        vtkMath::Subtract(original, cellNormal, shifted);
        points->InsertPoint(i + numberOfPoints, shifted);
        back.push_back(i + numberOfPoints);
    }

    polys->InsertNextCell(numberOfPoints, front.data());
    polys->InsertNextCell(numberOfPoints, back.data());

    m_selectedCellData->SetPoints(points);
    m_selectedCellData->SetPolys(polys);

    GetCurrentRenderer()->AddViewProp(m_selectedCellActor);
    GetCurrentRenderer()->GetRenderWindow()->Render();

    m_currentlyHighlighted = { dataObject, index };

    flashHightlightedCell();
}

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

void InteractorStyle3D::flashHightlightedCell(int milliseconds)
{
    if (!m_highlightFlashTimer)
    {
        m_highlightFlashTimer = new QTimer(this);
        m_highlightFlashTimer->setSingleShot(false);
        m_highlightFlashTimer->setInterval(1);

        m_highlightFlashTimer = new QTimer(this);
        connect(m_highlightFlashTimer, &QTimer::timeout,
            [this] () {
            int ms = (1000 + m_highlightFlashTime->msec() - QTime::currentTime().msec()) % 1000;
            if (*m_highlightFlashTime < QTime::currentTime())
            {
                m_highlightFlashTimer->stop();
                ms = 0;
            }
            double bg = 0.5 + 0.5 * std::cos(ms * 0.001 * 2.0 * vtkMath::Pi());
            m_selectedCellActor->GetProperty()->SetColor(1, bg, bg);
            GetCurrentRenderer()->GetRenderWindow()->Render();
        });
    }
    else if (m_highlightFlashTimer->isActive())
    {
        return;
    }

    *m_highlightFlashTime = QTime::currentTime().addMSecs(milliseconds);
    m_highlightFlashTimer->start();
}

void InteractorStyle3D::sendPointInfo() const
{
    vtkAbstractMapper3D * cellMapper = m_cellPicker->GetMapper();
    vtkAbstractMapper3D * pointMapper = m_pointPicker->GetMapper();

    if (!cellMapper && !pointMapper)    // no object at cursor position
    {
        emit pointInfoSent({});
        return;
    }

    auto mapper = cellMapper ? cellMapper : pointMapper;
    assert(mapper);

    auto dataObject = DataObject::getDataObject(*mapper->GetInformation());
    auto polyData = dynamic_cast<PolyDataObject *>(dataObject);

    // don't list unreferenced points for triangular data
    if (polyData && !cellMapper)
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
        << "Data Set: " << inputName << endl;

    // for poly data: centroid and scalar information if available
    if (polyData)
    {
        vtkIdType cellId = m_cellPicker->GetCellId();
        double centroid[3];
        polyData->cellCenters()->GetPoint(cellId, centroid);
        stream
            << "Triangle Index: " << cellId << endl
            << "X = " << centroid[0] << endl
            << "Y = " << centroid[1] << endl
            << "Z = " << centroid[2];

        vtkMapper * concreteMapper = vtkMapper::SafeDownCast(cellMapper);
        assert(concreteMapper);
        auto arrayName = concreteMapper->GetArrayName();
        vtkDataArray * scalars = arrayName ? polyData->processedDataSet()->GetCellData()->GetArray(arrayName) : nullptr;
        if (scalars)
        {
            auto component = concreteMapper->GetLookupTable()->GetVectorComponent();
            assert(component >= 0);
            double value =
                scalars->GetTuple(cellId)[component];

            stream  << endl << endl << "Attribute: " << QString::fromUtf8(arrayName) << " (" << + component << ")" << endl;
            stream << "Value: " << value;
        }
    }
    // for 3D vectors
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

    while (stream.readLineInto(&line))
        info.push_back(line);

    emit pointInfoSent(info);
}
