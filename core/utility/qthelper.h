#pragma once

#include <core/core_api.h>

#include <QString>

#include <vtkVector.h>


class QColor;
class QDebug;
class QEvent;


/** https://stackoverflow.com/questions/22535469/how-to-get-human-readable-event-type-from-qevent
  * Gives human-readable event type information. */
QDebug CORE_API operator<<(QDebug str, const QEvent * ev);

QColor CORE_API vtkColorToQColor(double colorF[3]);

template<typename T, int Size>
QString vtkVectorToQString(const vtkVector<T, Size> & vector,
    const QString & separator = " ", const QString & prefix = "", const QString & suffix = "");


#include "qthelper.hpp"
