#pragma once

#include <memory>

#include <QDialog>
#include <QPointer>

#include <gui/gui_api.h>


class QMenu;

class CoordinateTransformableDataObject;
struct ReferencedCoordinateSystemSpecification;
class Ui_CoordinateSystemAdjustmentWidget;


class GUI_API CoordinateSystemAdjustmentWidget : public QDialog
{
public:
    explicit CoordinateSystemAdjustmentWidget(QWidget * parent = nullptr, Qt::WindowFlags f = {});
    ~CoordinateSystemAdjustmentWidget() override;

    void setDataObject(CoordinateTransformableDataObject * dataObject);

private:
    void updateInfoText();
    void finish();

    ReferencedCoordinateSystemSpecification specFromUi() const;
    void specToUi(const ReferencedCoordinateSystemSpecification & spec);

private:
    std::unique_ptr<Ui_CoordinateSystemAdjustmentWidget> m_ui;
    std::unique_ptr<QMenu> m_autoSetReferencePointMenu;
    QAction * m_actionUnsupportedType;
    QAction * m_actionRefToNorthWest;
    QAction * m_actionRefCenter;
    QAction * m_actionRefOrigin;

    QPointer<CoordinateTransformableDataObject> m_dataObject;

private:
    Q_DISABLE_COPY(CoordinateSystemAdjustmentWidget)
};
