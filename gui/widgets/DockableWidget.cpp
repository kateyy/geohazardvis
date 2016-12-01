#include "DockableWidget.h"

#include <cassert>

#include <QDockWidget>
#include <QEvent>

#include <core/utility/macros.h>


DockableWidget::DockableWidget(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_dockWidgetParent{ nullptr }
    , m_closingReported{ false }
{
    setFocusPolicy(Qt::WheelFocus);
}

DockableWidget::~DockableWidget()
{
    assert(m_closingReported);
}

QDockWidget * DockableWidget::dockWidgetParent()
{
    if (m_dockWidgetParent)
    {
        return m_dockWidgetParent;
    }

    m_dockWidgetParent = new QDockWidget();
    m_dockWidgetParent->setWidget(this);
    m_dockWidgetParent->installEventFilter(this);
    m_dockWidgetParent->setFocusPolicy(Qt::WheelFocus);
    m_dockWidgetParent->setFocusProxy(this);
    m_dockWidgetParent->setWindowTitle(windowTitle());
    m_dockWidgetParent->setAttribute(Qt::WA_DeleteOnClose);
    // When QDockWidgets are tabbed and a different foreground widget is selected using the tab
    // labels, the QDockWidget is not automatically focused. Work around this using the
    // QDockWidget's visibilityChanged signal
    connect(m_dockWidgetParent, &QDockWidget::visibilityChanged, [this] (bool isVisible)
    {
        if (!isVisible || this->hasFocus())
        {
            return;
        }

        this->setFocus();
    });

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

bool DockableWidget::isClosed() const
{
    return m_closingReported;
}

void DockableWidget::signalClosing()
{
    if (m_closingReported)
    {
        return;
    }

    m_closingReported = true;

    emit closed();
}

bool DockableWidget::eventFilter(QObject * DEBUG_ONLY(obj), QEvent * ev)
{
    if (ev->type() == QEvent::Close)
    {
        assert(m_dockWidgetParent && obj == m_dockWidgetParent);

        // In case the dock widget is closed, detach (this) from the dock widget and let it be deleted in the user code.
        // To correctly emit closed() only once, trigger closeEvent() before detaching (this) from the dock widget.

        m_dockWidgetParent->setWidget(nullptr);
        this->setParent(nullptr);
        m_dockWidgetParent = nullptr;

        close();
    }

    return false;
}

void DockableWidget::closeEvent(QCloseEvent * event)
{
    signalClosing();

    // if (this) is closed directly, make sure to also close and delete the dock widget
    if (m_dockWidgetParent)
    {
        m_dockWidgetParent->close();
    }

    QWidget::closeEvent(event);
}
