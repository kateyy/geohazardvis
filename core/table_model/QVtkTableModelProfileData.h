#pragma once

#include <core/table_model/QVtkTableModel.h>


class DataProfile2DDataObject;


class CORE_API QVtkTableModelProfileData : public QVtkTableModel
{
public:
    explicit QVtkTableModelProfileData(QObject * parent = nullptr);
    ~QVtkTableModelProfileData() override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    IndexType indexType() const override;

protected:
    void resetDisplayData() override;

private:
    DataProfile2DDataObject * m_data;

private:
    Q_DISABLE_COPY(QVtkTableModelProfileData)
};
