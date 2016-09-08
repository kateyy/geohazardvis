#pragma once

#include <vtkSmartPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkDataArray;


class CORE_API QVtkTableModelRawVector : public QVtkTableModel
{
public:
    explicit QVtkTableModelRawVector(QObject * parent = nullptr);
    ~QVtkTableModelRawVector() override;
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

private:
    vtkSmartPointer<vtkDataArray> m_data;

private:
    Q_DISABLE_COPY(QVtkTableModelRawVector)
};
