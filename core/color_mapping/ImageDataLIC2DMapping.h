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

#include <QMap>

#include <core/color_mapping/ColorMappingData.h>


template<typename T> class QVector;
class vtkImageDataLIC2D;
class vtkRenderWindow;

class NoiseImageSource;


class CORE_API ImageDataLIC2DMapping : public ColorMappingData
{
public:
    explicit ImageDataLIC2DMapping(const QList<AbstractVisualizedData *> & visualizedData);
    ~ImageDataLIC2DMapping() override;

    QString name() const override;
    QString scalarsName(AbstractVisualizedData & vis) const override;
    IndexType scalarsAssociation(AbstractVisualizedData & vis) const override;

    vtkSmartPointer<vtkAlgorithm> createFilter(AbstractVisualizedData & visualizedData, int connection) override;
    bool usesFilter() const override;


protected:
    std::vector<ValueRange<>> updateBounds() override;

    vtkRenderWindow * glContext();

private:
    vtkSmartPointer<NoiseImageSource> m_noiseImage;
    QMap<AbstractVisualizedData *, QVector<vtkSmartPointer<vtkImageDataLIC2D>>> m_lic2D;

    vtkSmartPointer<vtkRenderWindow> m_glContext;

    static const QString s_name;
    static const bool s_isRegistered;
};
