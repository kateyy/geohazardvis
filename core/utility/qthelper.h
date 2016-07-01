#pragma once

#include <core/core_api.h>

#include <QMetaObject>


class QColor;
class QDebug;
class QEvent;
class QVariant;

class DataObject;


/** https://stackoverflow.com/questions/22535469/how-to-get-human-readable-event-type-from-qevent
  * Gives human-readable event type information. */
CORE_API QDebug operator<<(QDebug str, const QEvent * ev);

CORE_API QColor vtkColorToQColor(double colorF[4]);

/** Disconnect all connections and clear the list */
CORE_API void disconnectAll(QList<QMetaObject::Connection> & connections);
CORE_API void disconnectAll(QList<QMetaObject::Connection> && connections);

CORE_API QVariant dataObjectPtrToVariant(DataObject * dataObject);
CORE_API DataObject * variantToDataObjectPtr(const QVariant & variant);
