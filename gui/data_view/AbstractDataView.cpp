#include "AbstractDataView.h"

#include <QEvent>


AbstractDataView::AbstractDataView(
    int index, QWidget * parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
    , m_index(index)
    , m_initialized(false)
    , m_highlightedObject(nullptr)
    , m_hightlightedItemId(-1)
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

vtkIdType AbstractDataView::highlightedItemId() const
{
    return m_hightlightedItemId;
}

DataObject * AbstractDataView::highlightedObject()
{
    return m_highlightedObject;
}

const DataObject * AbstractDataView::highlightedObject() const
{
    return m_highlightedObject;
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

void AbstractDataView::setHighlightedId(DataObject * dataObject, vtkIdType itemId)
{
    if (m_highlightedObject == dataObject && m_hightlightedItemId == itemId)
        return;

    m_highlightedObject = dataObject;
    m_hightlightedItemId = itemId;

    highlightedIdChangedEvent(dataObject, itemId);
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
