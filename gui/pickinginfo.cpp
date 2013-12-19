#include "pickinginfo.h"

#include <vtkPointPicker.h>

void PickingInfo::sendPointInfo(vtkSmartPointer<vtkPointPicker> picker) const
{
    double* pos = picker->GetPickPosition();

    QString message = "Point: " + QString::number(pos[0]) + ":" + QString::number(pos[1]) + ":" + QString::number(pos[2])
        + ", id: " + QString::number(picker->GetPointId());

    emit infoSent(message);
}