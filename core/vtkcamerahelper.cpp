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

vtkVector2d operator-(const vtkVector2d & l, const vtkVector2d & r)
{
    return{ l.GetX() - r.GetX(), l.GetY() - r.GetY() };
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

double getVerticalElevation(vtkCamera * camera)
{
    vtkVector3d foc(camera->GetFocalPoint());
    vtkVector3d pos(camera->GetPosition());
    double viewXYLength = vtkVector2d((foc - pos).GetData()).Norm();
    double height = pos.GetZ() - foc.GetZ();

    return vtkMath::DegreesFromRadians(std::atan(height / viewXYLength));
}

void setVerticalElevation(vtkCamera * camera, double elevation)
{
    double radE = vtkMath::RadiansFromDegrees(elevation);

    double azimuth = getAzimuth(camera);
    setAzimuth(camera, 0);

    vtkVector2d focXZ(camera->GetFocalPoint()[0], camera->GetFocalPoint()[2]);
    vtkVector2d posXZ(camera->GetPosition()[0], camera->GetPosition()[2]);
    vtkVector2d invViewDirXZ(posXZ - focXZ);

    double viewLengthXZ2 = invViewDirXZ.SquaredNorm();

    double viewZ = std::sin(radE) * std::sqrt(viewLengthXZ2);
    double viewX = std::sqrt(viewLengthXZ2 - viewZ * viewZ);

    camera->SetPosition(
        focXZ.GetX() + viewX,
        camera->GetPosition()[1],
        focXZ.GetY() + viewZ);

    setAzimuth(camera, azimuth);
}

}
