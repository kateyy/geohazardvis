#include "DockableWidget.h"

#include <cassert>

#include <QDockWidget>
#include <QEvent>

#include <core/utility/macros.h>


DockableWidget::DockableWidget(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_dockWidgetParent(nullptr)
{
}

DockableWidget::~DockableWidget()
{
}

QDockWidget * DockableWidget::dockWidgetParent()
{
    if (m_dockWidgetParent)
        return m_dockWidgetParent;

    m_dockWidgetParent = new QDockWidget();
    m_dockWidgetParent->setWidget(this);
    m_dockWidgetParent->installEventFilter(this);
    m_dockWidgetParent->setFocusPolicy(Qt::StrongFocus);
    m_dockWidgetParent->setWindowTitle(windowTitle());
    m_dockWidgetParent->setAttribute(Qt::WA_DeleteOnClose);

    return m_dockWidgetParent;
}

QDockWidget * DockableWidget::dockWidgetParentOrNullptr()
{
    return m_dockWidgetParent;
}

bool DockableWidget::hasDockWidgetParent() const
{
    return m_dockWidgetParent != nullptr;
}

bool DockableWidget::eventFilter(QObject * DEBUG_ONLY(obj), QEvent * ev)
{
    if (ev->type() == QEvent::Close)
    {
        assert(m_dockWidgetParent && obj == m_dockWidgetParent);

        // In case the dock widget is closed, detach (this) from the dock widget and let it be deleted in the user code.
        // To correctly emit closed() only once, trigger closeEvent() before detaching (this) from the dock widget.
        close();

        m_dockWidgetParent->setWidget(nullptr);
        this->setParent(nullptr);
        m_dockWidgetParent = nullptr;
    }

    return false;
}

void DockableWidget::closeEvent(QCloseEvent * event)
{
    if (isVisible())
    {
        emit closed();
    }

    // if (this) is closed directly, make sure to also close and delete the dock widget
    if (m_dockWidgetParent)
    {
        m_dockWidgetParent->close();
    }

    QWidget::closeEvent(event);
}
