#pragma once

#include <vtkSmartPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkPolyData;


class CORE_API QVtkTableModelPolyData : public QVtkTableModel
{
public:
    QVtkTableModelPolyData(QObject * parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
    void resetDisplayData() override;

    vtkSmartPointer<vtkPolyData> m_vtkPolyData;
};
