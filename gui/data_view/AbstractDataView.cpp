#include "AbstractDataView.h"

#include <QEvent>


AbstractDataView::AbstractDataView(
    int index, QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_index(index)
    , m_initialized(false)
{
}

int AbstractDataView::index() const
{
    return m_index;
}

void AbstractDataView::updateTitle(QString message)
{
    QString title;

    if (message.isEmpty())
        title = friendlyName();
    else
        title = QString::number(index()) + ": " + message;

    setWindowTitle(title);
}

void AbstractDataView::showEvent(QShowEvent * /*event*/)
{
    if (m_initialized)
        return;

    contentWidget()->installEventFilter(this);

    m_initialized = true;
}

void AbstractDataView::focusInEvent(QFocusEvent * /*event*/)
{
    emit focused(this);
}

void AbstractDataView::setCurrent(bool isCurrent)
{
    auto f = font();
    f.setBold(isCurrent);
    setFont(f);
}

bool AbstractDataView::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    if (ev->type() == QEvent::FocusIn)
        emit focused(this);

    return false;
}

void AbstractDataView::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}
