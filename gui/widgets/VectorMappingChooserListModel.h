#pragma once

#include <QAbstractListModel>
#include <QList>


class VectorsToSurfaceMapping;
class VectorsForSurfaceMapping;


class VectorMappingChooserListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit VectorMappingChooserListModel(QObject * parent = nullptr);

    void setMapping(VectorsToSurfaceMapping * mapping);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;

signals:
    void vectorVisibilityChanged();

private:
    VectorsToSurfaceMapping * m_mapping;
    QStringList m_vectorNames;
    QList<VectorsForSurfaceMapping *> m_vectors;
};