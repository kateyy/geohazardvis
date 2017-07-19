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

#include <gui/data_view/RendererImplementationBase3D.h>


class RenderViewStrategy2D;


class GUI_API RendererImplementationResidual : public RendererImplementationBase3D
{
public:
    explicit RendererImplementationResidual(AbstractRenderView & renderView);
    ~RendererImplementationResidual() override;

    QString name() const override;

    void activate(t_QVTKWidget & qvtkWidget) override;

    void setShowModel(bool showModel);
    bool showModel() const;

    RenderViewStrategy2D & strategy2D();

protected:
    unsigned int subViewIndexAtPos(const QPoint pixelCoordinate) const override;

    RenderViewStrategy * strategyIfEnabled() const override;

private:
    void setupRenderers();

private:
    bool m_isInitialized;

    std::unique_ptr<RenderViewStrategy2D> m_strategy;

    bool m_showModel;

private:
    Q_DISABLE_COPY(RendererImplementationResidual)
};
