#pragma once

#include <QAbstractListModel>
#include <QList>


class GlyphMapping;
class GlyphMappingData;


class GlyphMappingChooserListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit GlyphMappingChooserListModel(QObject * parent = nullptr);
    ~GlyphMappingChooserListModel() override;

    void setMapping(GlyphMapping * mapping);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

signals:
    void glyphVisibilityChanged();

private:
    GlyphMapping * m_mapping;
    QStringList m_vectorNames;
    QList<GlyphMappingData *> m_vectors;

private:
    Q_DISABLE_COPY(GlyphMappingChooserListModel)
};
