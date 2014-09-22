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


namespace Camera
{

double getAzimuth(vtkCamera * camera)
{
    vtkVector3d foc(camera->GetFocalPoint());
    vtkVector3d pos(camera->GetPosition());
    vtkVector2d xyViewDir((foc - pos).GetData());
    double rad = std::acos(xyViewDir.GetX() / xyViewDir.Norm());
    return vtkMath::DegreesFromRadians(rad);
}

void setAzimuth(vtkCamera * camera, double azimuth)
{
    double radA = vtkMath::RadiansFromDegrees(azimuth);

    vtkVector3d foc(camera->GetFocalPoint());
    vtkVector3d pos(camera->GetPosition());
    double viewXYLength = vtkVector2d((foc - pos).GetData()).Norm();

    double inverseViewDirXY[2] {
        -std::cos(radA) * viewXYLength,
            std::sin(radA) * viewXYLength};

    camera->SetPosition(
        inverseViewDirXY[0] + camera->GetFocalPoint()[0],
        inverseViewDirXY[1] + camera->GetFocalPoint()[1],
        camera->GetPosition()[2]);
}

}
