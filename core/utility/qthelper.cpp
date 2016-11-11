#include "qthelper.h"

#include <cassert>
#include <type_traits>

#include <QColor>
#include <QDebug>
#include <QEvent>
#include <QList>
#include <QMetaEnum>
#include <QObject>


QDebug operator<<(QDebug str, const QEvent * ev)
{
    str << "QEvent";
    if (ev)
    {
        static int eventEnumIndex = QEvent::staticMetaObject.indexOfEnumerator("Type");
        const QString name = QEvent::staticMetaObject.enumerator(eventEnumIndex).valueToKey(ev->type());
        if (!name.isEmpty())
        {
            str << name;
        }
        else
        {
            str << ev->type();
        }
    }
    else
    {
        str << (void*)ev;
    }
    return str.maybeSpace();
}

QColor vtkColorToQColor(double colorF[4])
{
    return QColor(int(colorF[0] * 0xFF),
                  int(colorF[1] * 0xFF),
                  int(colorF[2] * 0xFF),
                  int(colorF[3] * 0xFF));
}

void disconnectAll(QList<QMetaObject::Connection> & connections)
{
    disconnectAll(std::move(connections));
}

void disconnectAll(QList<QMetaObject::Connection> && connections)
{
    for (auto && c : connections)
    {
        QObject::disconnect(c);
    }

    connections.clear();
}

QVariant dataObjectPtrToVariant(DataObject * dataObject)
{
    static_assert(sizeof(qulonglong) >= sizeof(size_t), "Not implemented for current architecture's pointer size.");

    return static_cast<qulonglong>(reinterpret_cast<size_t>(dataObject));
}

DataObject * variantToDataObjectPtr(const QVariant & variant)
{
    static_assert(sizeof(qulonglong) >= sizeof(DataObject *), "Not implemented for current architecture's pointer size.");

    bool okay;
    auto ptr = reinterpret_cast<DataObject *>(variant.toULongLong(&okay));
    assert(variant.isNull() || okay);
    return ptr;
}
