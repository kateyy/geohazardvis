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

#include <core/core_api.h>

#include <vtkSmartPointer.h>

#include <QPixmap>
#include <QString>


class vtkLookupTable;


class CORE_API GradientResourceManager
{
public:
    static GradientResourceManager & instance();

    void loadGradients();

    const QString & gradientsDir() const;

    struct GradientData
    {
        vtkSmartPointer<vtkLookupTable> lookupTable;
        QPixmap pixmap;
    };

    const std::map<QString, const GradientData> & gradients() const;
    const GradientData & gradient(const QString & name);

    const QString & defaultGradientName() const;
    void setDefaultGradientName(const QString & name);
    const GradientData & defaultGradient() const;

    void operator=(const GradientResourceManager &) = delete;

private:
    GradientResourceManager();
    ~GradientResourceManager();

    static vtkSmartPointer<vtkLookupTable> buildLookupTable(const QPixmap & pixmap);
    static GradientData buildFallbackGradient(const QSize & pixmapSize);

private:
    const QString m_gradientsDir;

    std::map<QString, const GradientData> m_gradients;
    QString m_defaultGradientName;
};
