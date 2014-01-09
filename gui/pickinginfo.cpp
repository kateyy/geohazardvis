#include "pickinginfo.h"

#include <QTextStream>
#include <QStringList>

#include <vtkPointPicker.h>

void PickingInfo::sendPointInfo(vtkSmartPointer<vtkPointPicker> picker) const
{
    double* pos = picker->GetPickPosition();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    stream << "selected point: " << endl
        << pos[0] << endl
        << pos[1] << endl
        << pos[2] << endl
        << "id: " << picker->GetPointId();

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit infoSent(info);
}
