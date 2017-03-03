#pragma once

#include <QGroupBox>

#if COMPILE_QT_DESIGNER_PLUGIN
#define GUI_API
#else
#include <gui/gui_api.h>
#endif


class GUI_API CollapsibleGroupBox : public QGroupBox
{
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

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
