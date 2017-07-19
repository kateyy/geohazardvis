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

#pragma once

#include <vector>

#include <QAbstractListModel>
#include <QStringList>


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
    std::vector<GlyphMappingData *> m_vectors;

private:
    Q_DISABLE_COPY(GlyphMappingChooserListModel)
};
