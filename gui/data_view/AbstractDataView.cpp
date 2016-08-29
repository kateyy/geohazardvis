#include "AbstractDataView.h"

#include <cassert>

#include <QDockWidget>
#include <QEvent>
#include <QLayout>
#include <QToolBar>

#include <gui/DataMapping.h>


AbstractDataView::AbstractDataView(
    DataMapping & dataMapping, int index, QWidget * parent, Qt::WindowFlags flags)
    : DockableWidget(parent, flags)
    , m_dataMapping{ dataMapping }
    , m_index{ index }
    , m_initialized{ false }
    , m_toolBar{ nullptr }
{
}

AbstractDataView::~AbstractDataView() = default;

DataMapping & AbstractDataView::dataMapping() const
{
    return m_dataMapping;
}

DataSetHandler & AbstractDataView::dataSetHandler() const
{
    return m_dataMapping.dataSetHandler();
}

int AbstractDataView::index() const
{
    return m_index;
}

void AbstractDataView::updateTitle(const QString & message)
{
    const auto title = message.isEmpty()
        ? friendlyName()
        : QString::number(index()) + ": " + message;

    if (title == windowTitle())
    {
        return;
    }

    setWindowTitle(title);
    if (hasDockWidgetParent())
    {
        dockWidgetParent()->setWindowTitle(title);
    }
}

QToolBar * AbstractDataView::toolBar()
{
    if (m_toolBar)
    {
        return m_toolBar;
    }

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
    {
        return false;
    }

    return m_toolBar->isVisible();
}

void AbstractDataView::setToolBarVisible(bool visible)
{
    if (!visible && !m_toolBar)
    {
        return;
    }

    toolBar()->setVisible(visible);
}

QString AbstractDataView::subViewFriendlyName(unsigned int /*subViewIndex*/) const
{
    return "";
}

void AbstractDataView::setSelection(const DataSelection & selection)
{
    if (selection == m_selection)
    {
        return;
    }

    m_selection = selection;

    onSetSelection(m_selection);

    emit selectionChanged(this, m_selection);
}

void AbstractDataView::clearSelection()
{
    if (m_selection.isEmpty())
    {
        return;
    }

    m_selection.clear();

    onClearSelection();

    emit selectionChanged(this, m_selection);
}

const DataSelection & AbstractDataView::selection() const
{
    return m_selection;
}

void AbstractDataView::showEvent(QShowEvent * /*event*/)
{
    if (m_initialized)
    {
        return;
    }

    contentWidget()->installEventFilter(this);

    m_initialized = true;
}

void AbstractDataView::focusInEvent(QFocusEvent * /*event*/)
{
    emit focused(this);
}

void AbstractDataView::setCurrent(bool isCurrent)
{
    auto mainWidget = hasDockWidgetParent() ? static_cast<QWidget *>(dockWidgetParent()) : this;
    auto f = mainWidget->font();
    f.setBold(isCurrent);
    mainWidget->setFont(f);
}

bool AbstractDataView::eventFilter(QObject * obj, QEvent * ev)
{
    const auto result = DockableWidget::eventFilter(obj, ev);

    if (ev->type() == QEvent::FocusIn)
    {
        emit focused(this);
    }

    return result;
}
