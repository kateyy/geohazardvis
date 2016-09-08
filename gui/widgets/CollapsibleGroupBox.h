#pragma once

#include <QGroupBox>

#include <gui/gui_api.h>


class GUI_API CollapsibleGroupBox : public QGroupBox
{
public:
    explicit CollapsibleGroupBox(QWidget * parent = nullptr);
    explicit CollapsibleGroupBox(const QString &title, QWidget * parent = nullptr);
    ~CollapsibleGroupBox() override;

    bool isCollapsed() const;

    void setCollapsed(bool collapsed);
    bool toggleCollapsed();

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

private:
    void applyCollapsed(bool collapsed);

private:
    Q_DISABLE_COPY(CollapsibleGroupBox)
};
