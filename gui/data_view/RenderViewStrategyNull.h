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

#include <gui/data_view/RenderViewStrategy.h>


class GUI_API RenderViewStrategyNull : public RenderViewStrategy
{
public:
    explicit RenderViewStrategyNull(RendererImplementationBase3D & context);
    ~RenderViewStrategyNull() override;

    QString name() const override;
    bool contains3dData() const override;
    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects, QList<DataObject *> & incompatibleObjects) const override;

private:
    Q_DISABLE_COPY(RenderViewStrategyNull)
};
