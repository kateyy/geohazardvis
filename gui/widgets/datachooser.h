#pragma once

#include <QDockWidget>

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

protected:
    Ui_DataChooser * m_ui;
};