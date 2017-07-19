/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GlyphMappingChooserListModel.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <core/glyph_mapping/GlyphMapping.h>
#include <core/glyph_mapping/GlyphMappingData.h>


GlyphMappingChooserListModel::GlyphMappingChooserListModel(QObject * parent)
    : QAbstractListModel(parent)
    , m_mapping{ nullptr }
{
}

GlyphMappingChooserListModel::~GlyphMappingChooserListModel() = default;

void GlyphMappingChooserListModel::setMapping(GlyphMapping * mapping)
{
    beginResetModel();

    m_mapping = mapping;

    if (m_mapping)
    {
        m_vectorNames = m_mapping->vectorNames();
        m_vectors = m_mapping->vectors();
        assert(static_cast<size_t>(m_vectorNames.size()) == m_vectors.size());
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

    return static_cast<int>(std::min(static_cast<size_t>(std::numeric_limits<int>::max()), m_mapping->vectors().size()));
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
