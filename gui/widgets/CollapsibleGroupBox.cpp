#include "CollapsibleGroupBox.h"

#include <QMouseEvent>
#include <QStyle>


CollapsibleGroupBox::CollapsibleGroupBox(QWidget * parent)
    : CollapsibleGroupBox("", parent)
{
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget * parent)
    : QGroupBox(title, parent)
{
    setAttribute(Qt::WA_SetStyle);
    setCheckable(true);

    setStyleSheet(
        "QGroupBox::indicator {"
        "width: 13px;"
        "height: 13px;"
        "}"

        "QGroupBox::indicator:checked {"
        "image: url(:/icons/plus_green.svg);"
        "}"

        "QGroupBox::indicator:unchecked {"
        "image: url(:/icons/minus_green.svg);"
        "}"
        );

    connect(this, &QGroupBox::toggled, this, &CollapsibleGroupBox::applyCollapsed);
}

CollapsibleGroupBox::~CollapsibleGroupBox() = default;

bool CollapsibleGroupBox::isCollapsed() const
{
    return isChecked();
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
    setChecked(collapsed);
}

bool CollapsibleGroupBox::toggleCollapsed()
{
    setCollapsed(!isCollapsed());

    return isCollapsed();
}

void CollapsibleGroupBox::applyCollapsed(bool collapsed)
{
    if (collapsed)
    {
        setFixedHeight(QWIDGETSIZE_MAX);
        adjustSize();
    }
    else
    {
        setFixedHeight(17);
    }
}

void CollapsibleGroupBox::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->pos().y() > 20)
    {
        return;
    }

    toggleCollapsed();
}
