#pragma once

#include <core/core_api.h>


class QDebug;
class QEvent;


/** https://stackoverflow.com/questions/22535469/how-to-get-human-readable-event-type-from-qevent
  * Gives human-readable event type information. */
QDebug CORE_API operator<<(QDebug str, const QEvent * ev);