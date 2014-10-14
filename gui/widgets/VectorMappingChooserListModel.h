#pragma once

#include <QAbstractListModel>
#include <QList>


class VectorMapping;
class VectorMappingData;


class VectorMappingChooserListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit VectorMappingChooserListModel(QObject * parent = nullptr);

    void setMapping(VectorMapping * mapping);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;

signals:
    void vectorVisibilityChanged();

private:
    VectorMapping * m_mapping;
    QStringList m_vectorNames;
    QList<VectorMappingData *> m_vectors;
};
