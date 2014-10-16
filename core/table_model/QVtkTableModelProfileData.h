#pragma once

#include <core/table_model/QVtkTableModel.h>


class ImageProfileData;


class CORE_API QVtkTableModelProfileData : public QVtkTableModel
{
public:
    QVtkTableModelProfileData(QObject * parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

protected:
    void resetDisplayData() override;

private:
    ImageProfileData * m_data;
};
