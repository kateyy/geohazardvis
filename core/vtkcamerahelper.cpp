#include "vtkcamerahelper.h"

#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkVector.h>


namespace
{

vtkVector3d operator-(const vtkVector3d & l, const vtkVector3d & r)
{
    return{ l.GetX() - r.GetX(), l.GetY() - r.GetY(), l.GetZ() - r.GetZ() };
}

}


namespace TerrainCamera
{

double getAzimuth(vtkCamera * camera)
{
    vtkVector3d foc(camera->GetFocalPoint());
    vtkVector3d pos(camera->GetPosition());
    vtkVector2d xyViewDir((foc - pos).GetData());
    double rad = std::atan2(xyViewDir.GetY(), xyViewDir.GetX());
    return std::fmod(vtkMath::DegreesFromRadians(rad) + 360, 360.0);
}

void setAzimuth(vtkCamera * camera, double azimuth)
{
    double radA = vtkMath::RadiansFromDegrees(azimuth);

    vtkVector3d foc(camera->GetFocalPoint());
    vtkVector3d pos(camera->GetPosition());
    double viewXYLength = vtkVector2d((foc - pos).GetData()).Norm();

    double inverseViewDirXY[2] {
        -std::cos(radA) * viewXYLength,
        -std::sin(radA) * viewXYLength};

    camera->SetPosition(
        inverseViewDirXY[0] + foc.GetX(),
        inverseViewDirXY[1] + foc.GetY(),
        camera->GetPosition()[2]);
}

}
