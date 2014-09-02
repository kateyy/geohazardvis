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

void AbstractDataView::showEvent(QShowEvent * /*event*/)
{
    if (m_initialized)
        return;

    contentWidget()->installEventFilter(this);

    m_initialized = true;
}

void AbstractDataView::focusInEvent(QFocusEvent * /*event*/)
{
    auto f = font();
    f.setBold(true);
    setFont(f);

    emit focused(this);
}

void AbstractDataView::focusOutEvent(QFocusEvent * /*event*/)
{
    if (contentWidget()->hasFocus())
        return;

    auto f = font();
    f.setBold(false);
    setFont(f);
}

bool AbstractDataView::eventFilter(QObject * /*obj*/, QEvent * ev)
{
    if (ev->type() == QEvent::FocusIn)
        setFocus();

    return false;
}

void AbstractDataView::closeEvent(QCloseEvent * event)
{
    emit closed();

    QDockWidget::closeEvent(event);
}
