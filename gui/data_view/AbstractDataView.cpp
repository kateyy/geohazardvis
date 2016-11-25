#include "AbstractDataView.h"

#include <cassert>
#include <tuple>

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
    , m_friendlyName{}
    , m_activeSubViewIndex{ 0u }
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

const QString & AbstractDataView::friendlyName()
{
    if (m_friendlyName.isNull())
    {
        std::tie(m_friendlyName, m_subViewFriendlyNames) = friendlyNameInternal();
        if (m_friendlyName.isNull())
        {
            m_friendlyName = "";
        }
    }

    return m_friendlyName;
}

const QString & AbstractDataView::subViewFriendlyName(unsigned int subViewIndex)
{
    static const QString emptyName = "";
    friendlyName(); // trigger update if required

    assert(subViewIndex <= numberOfSubViews());

    return subViewIndex < m_subViewFriendlyNames.size()
        ? m_subViewFriendlyNames[subViewIndex]
        : emptyName;
}

unsigned int AbstractDataView::numberOfSubViews() const
{
    return 1;
}

unsigned int AbstractDataView::activeSubViewIndex() const
{
    return m_activeSubViewIndex;
}

void AbstractDataView::setActiveSubView(unsigned int subViewIndex)
{
    assert(subViewIndex < numberOfSubViews());
    if (subViewIndex >= numberOfSubViews())
    {
        return;
    }

    if (m_activeSubViewIndex == subViewIndex)
    {
        return;
    }

    m_activeSubViewIndex = subViewIndex;

    activeSubViewChangedEvent(m_activeSubViewIndex);

    emit activeSubViewChanged(m_activeSubViewIndex);
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

void AbstractDataView::activeSubViewChangedEvent(unsigned int /*subViewIndex*/)
{
}

void AbstractDataView::resetFriendlyName()
{
    m_friendlyName = QString();
    m_subViewFriendlyNames.clear();
}
