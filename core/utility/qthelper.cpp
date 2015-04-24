#include "qthelper.h"

#include <QDebug>
#include <QEvent>
#include <QMetaEnum>


QDebug operator<<(QDebug str, const QEvent * ev)
{
    static int eventEnumIndex = QEvent::staticMetaObject
        .indexOfEnumerator("Type");
    str << "QEvent";
    if (ev)
    {
        QString name = QEvent::staticMetaObject
            .enumerator(eventEnumIndex).valueToKey(ev->type());
        if (!name.isEmpty()) str << name; else str << ev->type();
    }
    else
    {
        str << (void*)ev;
    }
    return str.maybeSpace();
}
