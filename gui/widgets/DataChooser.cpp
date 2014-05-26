#include "DataChooser.h"
#include "ui_DataChooser.h"


DataChooser::DataChooser(QWidget * parent)
: QDockWidget(parent)
, m_ui(new Ui_DataChooser())
{
    m_ui->setupUi(this);
}

DataChooser::~DataChooser()
{
    delete m_ui;
}

void DataChooser::updateSelection()
{
    emit selectionChanged(dataSelection());
}

DataSelection DataChooser::dataSelection() const
{
    if (m_ui->scalars_singleColor->isChecked())
        return DataSelection::DefaultColor;
    if (m_ui->scalars_xValues->isChecked())
        return DataSelection::Vertex_xValues;
    if (m_ui->scalars_yValues->isChecked())
        return DataSelection::Vertex_yValues;
    if (m_ui->scalars_zValues->isChecked())
        return DataSelection::Vertex_zValues;

    return DataSelection::NoSelection;
}
