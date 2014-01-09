#pragma once

#include <QObject>
#include <vtkSmartPointer.h>

class vtkPointPicker;

class QStringList;

class PickingInfo : public QObject
{
    Q_OBJECT

public:
    void sendPointInfo(vtkSmartPointer<vtkPointPicker> picker) const;

signals:
    void infoSent(const QStringList &info) const;
};
