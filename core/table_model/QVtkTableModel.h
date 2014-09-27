#pragma once

#include <QAbstractTableModel>

#include <core/core_api.h>


class DataObject;


class CORE_API QVtkTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    QVtkTableModel(QObject * parent = nullptr);

    void setDataObject(DataObject * dataObject);
    DataObject * dataObject();

protected:
    virtual void resetDisplayData() = 0;

private:
    DataObject * m_dataObject;
};
