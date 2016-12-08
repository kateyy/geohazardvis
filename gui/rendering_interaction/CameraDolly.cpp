#include <gui/rendering_interaction/CameraDolly.h>

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QThread>

#include <vtkAlgorithm.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCamera.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolygon.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <core/AbstractVisualizedData.h>
#include <core/types.h>
#include <core/data_objects/DataObject.h>
#include <core/utility/mathhelper.h>
#include <core/utility/vtkcamerahelper.h>


using namespace mathhelper;


void CameraDolly::setRenderer(vtkRenderer * renderer)
{
    m_renderer = renderer;
}

vtkRenderer * CameraDolly::renderer() const
{
    return m_renderer;
}

void CameraDolly::moveTo(AbstractVisualizedData & visualization, vtkIdType index, IndexType indexType, bool overTime)
{
    if (!m_renderer)
    {
        return;}

    if (visualization.numberOfOutputPorts() == 0)
    {
        qDebug() << "[CameraDolly] Visualization has no outputs";
        return;
    }

    auto dataSet = visualization.processedOutputDataSet(visualization.defaultOutputPort());

    if (!dataSet)
    {
        qDebug() << "[CameraDolly] No data output found in visualization output port";
        return;
    }

    moveTo(*dataSet, index, indexType, overTime);
}

void CameraDolly::moveTo(DataObject & dataObject, vtkIdType index, IndexType indexType, bool overTime)
{
    auto dataSet = dataObject.dataSet();

    if (!dataSet)
    {
        qDebug() << "No data found in data object: " << dataObject.name();
        return;
    }

    moveTo(*dataSet, index, indexType, overTime);
}

void CameraDolly::moveTo(vtkDataSet & dataSet, vtkIdType index, IndexType indexType, bool overTime)
{
    if (!m_renderer)
    {
        return;
    }

    if (auto poly = vtkPolyData::SafeDownCast(&dataSet))
    {
        moveToPoly(*poly, index, indexType, overTime);
        return;
    }
    if (auto image = vtkImageData::SafeDownCast(&dataSet))
    {
        moveToImage(*image, index, indexType, overTime);
        return;
    }
}

void CameraDolly::moveToPoly(vtkPolyData & polyData, vtkIdType index, IndexType indexType, bool overTime)
{
    if (!m_renderer)
    {
        return;
    }

    double selectionPoint[3], selectionNormal[3];

    if (indexType == IndexType::cells)
    {
        vtkCell * cell = polyData.GetCell(index);
        assert(cell);
        if (!cell)
        {
            qDebug() << "[CameraDolly] Cell not found in data set: " + QString::number(index);
            return;
        }

        auto cellPointIds = vtkSmartPointer<vtkIdTypeArray>::New();
        cellPointIds->SetArray(cell->GetPointIds()->GetPointer(0), cell->GetNumberOfPoints(), true);
        vtkPolygon::ComputeCentroid(cellPointIds, polyData.GetPoints(), selectionPoint);
        vtkPolygon::ComputeNormal(cell->GetPoints(), selectionNormal);
    }
    else
    {
        polyData.GetPoint(index, selectionPoint);
        auto normals = polyData.GetPointData()->GetNormals();
        if (!normals)
        {
            qDebug() << "[CameraDolly] No point normals found in poly data set";
            return;
        }

        normals->GetTuple(index, selectionNormal);
    }


    vtkCamera & camera = *m_renderer->GetActiveCamera();

    double objectCenter[3];
    polyData.GetCenter(objectCenter);

    double startingFocalPoint[3], targetFocalPoint[3];
    double startingAzimuth, targetAzimuth;
    double startingElevation, targetElevation;

    camera.GetFocalPoint(startingFocalPoint);
    for (size_t i = 0; i < 3; ++i)  // look at center of the object
    {
        targetFocalPoint[i] = objectCenter[i];
    }
    startingAzimuth = TerrainCamera::getAzimuth(camera);
    startingElevation = TerrainCamera::getVerticalElevation(camera);


    // compute target camera position to find azimuth and elevation for the transition
    double startingPosition[3];
    camera.GetPosition(startingPosition);

    double targetPositionXY[2];

    double objectToEye[3];
    vtkMath::Subtract(startingPosition, objectCenter, objectToEye);
    double viewDistanceXY = vtkMath::Normalize2D(objectToEye);

    double selectionCenterXY[2] = { selectionPoint[0], selectionPoint[1] };

    double norm_objectToSelectionXY[2];
    norm_objectToSelectionXY[0] = selectionCenterXY[0] - objectCenter[0];
    norm_objectToSelectionXY[1] = selectionCenterXY[1] - objectCenter[1];
    double selectionRadiusXY = vtkMath::Normalize2D(norm_objectToSelectionXY);

    // make sure to move outside of the selection
    if (viewDistanceXY < selectionRadiusXY)
    {
        viewDistanceXY = selectionRadiusXY * 1.5;
    }


    // choose nearest viewpoint for flat surfaces
    const double flat_threshold = 30;
    double inclination = std::acos(selectionNormal[2]) * 180.0 / vtkMath::Pi();
    if (inclination < flat_threshold)
    {
        targetPositionXY[0] = objectCenter[0] + viewDistanceXY * norm_objectToSelectionXY[0];
        targetPositionXY[1] = objectCenter[1] + viewDistanceXY * norm_objectToSelectionXY[1];
    }
    // or use the hill's normal
    else
    {
        double selectionNormalXY[2] = { selectionNormal[0], selectionNormal[1] };

        double l;
        if ((l = std::sqrt((selectionNormalXY[0] * selectionNormalXY[0] + selectionNormalXY[1] * selectionNormalXY[1]))) != 0.0)
        {
            selectionNormalXY[0] /= l;
            selectionNormalXY[1] /= l;
        }

        // get a point in front of the selected cell
        double selectionFrontXY[2] = { selectionCenterXY[0] + selectionNormalXY[0], selectionCenterXY[1] + selectionNormalXY[1] };

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
            {
                intersectionIndex = 0;
            }
            else
            {
                intersectionIndex = 2;
            }
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

    if (overTime)
    {

        const double flyTimeSec = 0.5;
        const int flyDeadlineMSec = 1000;

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
        QTime deadline = startingTime.addMSecs(flyDeadlineMSec);
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

            m_renderer->ResetCameraClippingRange();
            m_renderer->GetRenderWindow()->Render();

            if (QTime::currentTime() > deadline)
            {
                break;
            }

            QThread::msleep((unsigned long)std::max(0l, sleepMSec - renderTime.elapsed()));
        }
    }

    // in any case, jump to target position

    camera.SetFocalPoint(targetFocalPoint);
    camera.SetPosition(targetCamera->GetPosition());
    TerrainCamera::setAzimuth(camera, targetAzimuth);
    TerrainCamera::setVerticalElevation(camera, targetElevation);
    m_renderer->ResetCameraClippingRange();
    m_renderer->GetRenderWindow()->Render();
}

void CameraDolly::moveToImage(vtkImageData & /*image*/, vtkIdType /*index*/, IndexType /*indexType*/, bool /*overTime*/)
{
    // Not implemented yet
}
