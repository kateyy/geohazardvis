#pragma once

#include <QDockWidget>
#include <QList>

#include "core/data_mapping/ScalarToColorMapping.h"


class RenderedData;
class Ui_DataChooser;

class DataChooser : public QDockWidget
{
    Q_OBJECT

public:
    DataChooser(QWidget * parent = nullptr);
    ~DataChooser() override;

    /** setup UI for a named rendered and a configured scalar to color mapping */
    void setMapping(QString rendererName = "", ScalarToColorMapping * mapping = nullptr);
    const ScalarToColorMapping * mapping() const;

    const QImage & selectedGradient() const;

signals:
    void renderSetupChanged();

private slots:
    void on_scalarsSelectionChanged(QString scalarsName);
    void on_gradientSelectionChanged(int selection);

private:
    void loadGradientImages();
    void updateWindowTitle(QString objectName = "");

private:
    Ui_DataChooser * m_ui;

    QList<QImage> m_scalarToColorGradients;

    ScalarToColorMapping * m_mapping;
};
