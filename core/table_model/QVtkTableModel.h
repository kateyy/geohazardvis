#pragma once

#include <QAbstractTableModel>

#include <vtkType.h>

#include <core/core_api.h>


class DataObject;


class CORE_API QVtkTableModel : public QAbstractTableModel
{
public:
    QVtkTableModel(QObject * parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setDataObject(DataObject * dataObject);
    DataObject * dataObject();

    vtkIdType hightlightItemId() const;

    /** @return cell/point/etc id for a table cell. This is the row by default. */
    virtual vtkIdType itemIdAt(const QModelIndex & index) const;

public:
    void setHighlightItemId(vtkIdType id);

protected:
    virtual void resetDisplayData() = 0;

private:
    DataObject * m_dataObject;
    vtkIdType m_hightlightId;
};
