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

#include <gui/data_view/RendererImplementationBase3D.h>


class RenderViewStrategySwitch;


class GUI_API RendererImplementation3D : public RendererImplementationBase3D
{
public:
    explicit RendererImplementation3D(AbstractRenderView & renderView);
    ~RendererImplementation3D() override;

    QString name() const override;

    QList<DataObject *> filterCompatibleObjects(const QList<DataObject *> & dataObjects,
        QList<DataObject *> & incompatibleObjects) override;

    void activate(t_QVTKWidget & qvtkWidget) override;

protected:
    void onRemoveContent(AbstractVisualizedData * content, unsigned int subViewIndex) override;

    void updateForCurrentInteractionStrategy(const QString & strategyName) override;
    RenderViewStrategy * strategyIfEnabled() const override;


private:
    void updateStrategies(const QList<DataObject *> & newDataObjects = {});
    QString mostSuitableStrategy(const QList<DataObject *> & newDataObjects) const;

private:
    std::map<QString, std::unique_ptr<RenderViewStrategy>> m_strategies;
    RenderViewStrategy * m_currentStrategy;

    static bool s_isRegistered;

private:
    Q_DISABLE_COPY(RendererImplementation3D)
};
