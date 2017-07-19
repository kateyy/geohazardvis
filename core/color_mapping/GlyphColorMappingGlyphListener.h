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

#include <QObject>


class AbstractVisualizedData;
class RenderedData3D;


class GlyphColorMappingGlyphListener : public QObject
{
    Q_OBJECT

public:
    explicit GlyphColorMappingGlyphListener(QObject * parent = nullptr);
    ~GlyphColorMappingGlyphListener() override;

    void setData(const std::vector<AbstractVisualizedData *> & visualizedData);

signals:
    void glyphMappingChanged();

private:
    std::vector<RenderedData3D *> m_data;
    std::vector<QMetaObject::Connection> m_connects;

private:
    Q_DISABLE_COPY(GlyphColorMappingGlyphListener)
};
