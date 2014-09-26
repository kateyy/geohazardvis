#include "QVtkTableModel.h"


QVtkTableModel::QVtkTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_dataObject(nullptr)
{
}

void QVtkTableModel::setDataObject(DataObject * dataObject)
{
    if (m_dataObject == dataObject)
        return;

    beginResetModel();

    m_dataObject = dataObject;

    resetDisplayData();

    endResetModel();
}

DataObject * QVtkTableModel::dataObject()
{
    return m_dataObject;
}
