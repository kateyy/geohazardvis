#include "qthelper.h"

#include <QColor>
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

QColor vtkColorToQColor(double colorF[4])
{
    return QColor(int(colorF[0] * 0xFF),
                  int(colorF[1] * 0xFF),
                  int(colorF[2] * 0xFF),
                  int(colorF[3] * 0xFF));
}
