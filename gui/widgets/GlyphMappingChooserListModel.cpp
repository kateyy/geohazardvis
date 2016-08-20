#include "GlyphMappingChooserListModel.h"

#include <cassert>

#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


GlyphMappingChooserListModel::GlyphMappingChooserListModel(QObject * parent)
    : QAbstractListModel(parent)
    , m_mapping{ nullptr }
{
}

void GlyphMappingChooserListModel::setMapping(GlyphMapping * mapping)
{
    beginResetModel();

    m_mapping = mapping;

    if (m_mapping)
    {
        m_vectorNames = m_mapping->vectorNames();
        m_vectors = m_mapping->vectors();
        assert(m_vectorNames.size() == m_vectors.size());
    }
    else
    {
        m_vectorNames.clear();
        m_vectors.clear();
    }

    endResetModel();
}

QVariant GlyphMappingChooserListModel::data(const QModelIndex & index, int role /*= Qt::DisplayRole*/) const
{
    int row = index.row();

    switch (role)
    {
    case Qt::ItemDataRole::DisplayRole:
        return m_mapping->vectorNames()[row];
    case Qt::ItemDataRole::CheckStateRole:
        return m_vectors[row]->isVisible()
            ? Qt::CheckState::Checked
            : Qt::CheckState::Unchecked;
    }

    return QVariant();
}

QVariant GlyphMappingChooserListModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ /*= Qt::DisplayRole*/) const
{
    return QVariant();
}

int GlyphMappingChooserListModel::rowCount(const QModelIndex & /*parent*/) const
{
    if (!m_mapping)
    {
        return 0;
    }

    return m_mapping->vectors().size();
}

bool GlyphMappingChooserListModel::setData(const QModelIndex & index, const QVariant & value, int role /*= Qt::EditRole*/)
{
    if (role == Qt::ItemDataRole::CheckStateRole)
    {
        bool ok;
        Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt(&ok));
        assert(ok);
        if (!ok)
        {
            return false;
        }

        m_vectors[index.row()]->setVisible(state == Qt::Checked);

        emit glyphVisibilityChanged();

        emit dataChanged(index, index, { Qt::ItemDataRole::CheckStateRole });

        return true;
    }

    return false;
}

Qt::ItemFlags GlyphMappingChooserListModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
