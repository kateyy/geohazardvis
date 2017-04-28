#pragma once

#include <vtkWeakPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkDataArray;
class GenericPolyDataObject;


/** Table model representing coordinate information of polygonal data or point clouds. */
class CORE_API QVtkTableModelPointCloudData : public QVtkTableModel
{
public:
    explicit QVtkTableModelPointCloudData(QObject * parent = nullptr);
    ~QVtkTableModelPointCloudData() override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    IndexType indexType() const override;

protected:
    void resetDisplayData() override;

    vtkDataArray * scalars() const;

private:
    GenericPolyDataObject * m_data;
    vtkWeakPointer<vtkDataArray> m_lastScalars;

private:
    Q_DISABLE_COPY(QVtkTableModelPointCloudData)
};
