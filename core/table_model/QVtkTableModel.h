#pragma once

#include <vector>

#include <QAbstractTableModel>

#include <vtkType.h>

#include <core/core_api.h>


class DataObject;
enum class IndexType;


class CORE_API QVtkTableModel : public QAbstractTableModel
{
public:
    explicit QVtkTableModel(QObject * parent = nullptr);
    ~QVtkTableModel() override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setDataObject(DataObject * dataObject);
    DataObject * dataObject();

    vtkIdType hightlightItemId() const;
    /** @return General association of listed positions and attributes (points vs. cells) */
    virtual IndexType indexType() const = 0;
    void setHighlightItemId(vtkIdType id);

    /** @return cell/point/etc id for a table cell. This is the row by default. */
    virtual vtkIdType itemIdAt(const QModelIndex & index) const;


protected:
    virtual void resetDisplayData() = 0;

private:
    void rebuild();

private:
    DataObject * m_dataObject;
    vtkIdType m_hightlightId;

    std::vector<QMetaObject::Connection> m_dataObjectConnections;

private:
    Q_DISABLE_COPY(QVtkTableModel)
};
