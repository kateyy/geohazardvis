#pragma once

#include <vtkSmartPointer.h>

#include <core/table_model/QVtkTableModel.h>


class vtkImageData;


class CORE_API QVtkTableModelImage : public QVtkTableModel
{
public:
    explicit QVtkTableModelImage(QObject * parent = nullptr);
    ~QVtkTableModelImage() override;
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

    void tableToImageCoord(int tableRow, int & imageRow, int & imageColumn) const;

private:
    vtkSmartPointer<vtkImageData> m_vtkImageData;

private:
    Q_DISABLE_COPY(QVtkTableModelImage)
};
