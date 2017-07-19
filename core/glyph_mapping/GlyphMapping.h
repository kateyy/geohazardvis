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

#include <map>
#include <memory>
#include <vector>

#include <QObject>

#include <core/core_api.h>


class QString;
class QStringList;

class RenderedData;
class GlyphMappingData;


/**
Sets up vector to surface mapping for a data object and stores the configuration state.
Uses GlyphMappingData to determine vectors that can be mapped on the supplied data.
*/
class CORE_API GlyphMapping : public QObject
{
    Q_OBJECT

public:
    explicit GlyphMapping(RenderedData & renderedData);
    ~GlyphMapping() override;

    /** names of vectors that can be used with my data */
    QStringList vectorNames() const;
    /** list of vector data that can be used with my data */
    std::vector<GlyphMappingData *> vectors() const;

    const RenderedData & renderedData() const;

signals:
    void vectorsChanged();

private:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableVectors();

private:
    RenderedData & m_renderedData;

    std::map<QString, std::unique_ptr<GlyphMappingData>> m_vectors;
};
