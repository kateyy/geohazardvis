#pragma once

#include <QDockWidget>
#include <QVector>


class Ui_DataChooser;


enum class DataSelection
{
    NoSelection,
    DefaultColor,
    Vertex_xValues,
    Vertex_yValues,
    Vertex_zValues
};

class DataChooser : public QDockWidget
{
    Q_OBJECT

public:
    DataChooser(QWidget * parent = nullptr);
    ~DataChooser() override;

    DataSelection dataSelection() const;

    const QImage & selectedGradient() const;

signals:
    void selectionChanged(DataSelection selection);
    void gradientSelectionChanged(const QImage & currentGradient);

private slots:
    /// read current gui selection and emit selectionChanged() accordingly
    void updateSelection();
    void updateGradientSelection(int selection);

private:
    void loadGradientImages();

private:
    Ui_DataChooser * m_ui;

    QVector<QImage> m_scalarToColorGradients;
};
