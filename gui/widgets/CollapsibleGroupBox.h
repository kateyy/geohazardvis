#pragma once

#include <QGroupBox>

#include <gui/gui_api.h>


class GUI_API CollapsibleGroupBox : public QGroupBox
{
public:
    explicit CollapsibleGroupBox(QWidget * parent = nullptr);
    explicit CollapsibleGroupBox(const QString &title, QWidget * parent = nullptr);

    bool isCollapsed() const;

public slots:
    void setCollapsed(bool collapsed);
    bool toggleCollapsed();

private slots:
    void applyCollapsed(bool collapsed);

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

};
