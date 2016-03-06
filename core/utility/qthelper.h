#pragma once

#include <core/core_api.h>

#include <QMetaObject>


class QColor;
class QDebug;
class QEvent;


/** https://stackoverflow.com/questions/22535469/how-to-get-human-readable-event-type-from-qevent
  * Gives human-readable event type information. */
QDebug CORE_API operator<<(QDebug str, const QEvent * ev);

QColor CORE_API vtkColorToQColor(double colorF[3]);

/** Disconnect all connections and clear the list */
void CORE_API disconnectAll(QList<QMetaObject::Connection> & connections);
void CORE_API disconnectAll(QList<QMetaObject::Connection> && connections);
