#pragma once

#include <memory>

#include <QDialog>
#include <QPointer>
#include <QVector>

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

    void setupQuickSetActions();

    ReferencedCoordinateSystemSpecification specFromUi() const;
    void specToUi(const ReferencedCoordinateSystemSpecification & spec);

private:
    std::unique_ptr<Ui_CoordinateSystemAdjustmentWidget> m_ui;
    std::unique_ptr<QMenu> m_autoSetReferencePointMenu;
    QVector<QAction *> m_refPointQuickSetActions;

    QPointer<CoordinateTransformableDataObject> m_dataObject;

private:
    Q_DISABLE_COPY(CoordinateSystemAdjustmentWidget)
};
