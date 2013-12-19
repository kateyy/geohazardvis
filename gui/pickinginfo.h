#pragma once

#include <QObject>
#include <vtkSmartPointer.h>

class vtkPointPicker;

class PickingInfo : public QObject
{
    Q_OBJECT

public:
    void sendPointInfo(vtkSmartPointer<vtkPointPicker> picker) const;

signals:
    void infoSent(QString info) const;
};
