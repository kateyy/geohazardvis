#pragma once

#include <memory>
#include <vector>

#include <QWidget>

#include <gui/gui_api.h>


class Ui_ResidualViewConfigWidget;
class ResidualVerificationView;


class GUI_API ResidualViewConfigWidget : public QWidget
{
public:
    explicit ResidualViewConfigWidget(QWidget * parent = nullptr);
    ~ResidualViewConfigWidget() override;

    void setCurrentView(ResidualVerificationView * view);

private:
    void updateComboBoxes();

    void updateObservationFromUi(int index);
    void updateModelFromUi(int index);

private:
    std::unique_ptr<Ui_ResidualViewConfigWidget> m_ui;

    ResidualVerificationView * m_currentView;
    std::vector<QMetaObject::Connection> m_viewConnects;

private:
    Q_DISABLE_COPY(ResidualViewConfigWidget)
};
