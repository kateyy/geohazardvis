#include "AbstractDataView.h"

#include <QDockWidget>
#include <QEvent>
#include <QLayout>
#include <QToolBar>


AbstractDataView::AbstractDataView(
    int index, QWidget * parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
    , m_index(index)
    , m_initialized(false)
    , m_dockWidgetParent(nullptr)
    , m_toolBar(nullptr)
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
    if (m_dockWidgetParent)
        m_dockWidgetParent->setWindowTitle(title);
}

QDockWidget * AbstractDataView::dockWidgetParent()
{
    if (m_dockWidgetParent)
        return m_dockWidgetParent;

    m_dockWidgetParent = new QDockWidget();
    m_dockWidgetParent->setWidget(this);
    m_dockWidgetParent->installEventFilter(this);
    m_dockWidgetParent->setFocusPolicy(Qt::StrongFocus);

    return m_dockWidgetParent;
}

bool AbstractDataView::hasDockWidgetParent() const
{
    return m_dockWidgetParent != nullptr;
}

QToolBar * AbstractDataView::toolBar()
{
    if (m_toolBar)
        return m_toolBar;

    m_toolBar = new QToolBar();
    auto font = m_toolBar->font();
    font.setBold(false);
    m_toolBar->setFont(font);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);

    layout()->addWidget(m_toolBar);

    return m_toolBar;
}

bool AbstractDataView::toolBarIsVisible() const
{
    if (!m_toolBar)
        return false;

    return m_toolBar->isVisible();
}

void AbstractDataView::setToolBarVisible(bool visible)
{
    if (!visible && !m_toolBar)
        return;

    toolBar()->setVisible(visible);
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
    auto mainWidget = m_dockWidgetParent ? (QWidget *) m_dockWidgetParent : this;
    auto f = mainWidget->font();
    f.setBold(isCurrent);
    mainWidget->setFont(f);
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

    if (ev->type() == QEvent::Close)
        close();

    return false;
}

void AbstractDataView::closeEvent(QCloseEvent * event)
{
    if (isVisible())
        emit closed();

    QWidget::closeEvent(event);
}
