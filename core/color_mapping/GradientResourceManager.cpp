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

#include "GradientResourceManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QPixmap>

#include <vtkLookupTable.h>

#include <core/RuntimeInfo.h>
#include <core/utility/qthelper.h>


namespace
{
// Transparent NaN-color currently not correctly supported, thus a=1 for now.
// Note: VTK interfaces don't allow const parameters in many cases
/*const*/ double l_defaultNanColor[] = { 1, 1, 1, 1 };
}

auto GradientResourceManager::gradients() const -> const std::map<QString, const GradientData> &
{
    return m_gradients;
}

auto GradientResourceManager::gradient(const QString & name) -> const GradientData &
{
    assert(!m_gradients.empty());

    auto it = m_gradients.find(name);
    if (it == m_gradients.end())
    {
        qWarning() << "Requested gradient not found:" << name;
        return defaultGradient();
    }

    return it->second;
}

const QString & GradientResourceManager::defaultGradientName() const
{
    return m_defaultGradientName;
}

void GradientResourceManager::setDefaultGradientName(const QString & name)
{
    m_defaultGradientName = name;
}

auto GradientResourceManager::defaultGradient() const -> const GradientData &
{
    assert(!m_gradients.empty());

    auto it = m_gradients.find(defaultGradientName());
    if (it != m_gradients.end())
    {
        return it->second;
    }

    return m_gradients.begin()->second;
}

GradientResourceManager::GradientResourceManager()
    : m_gradientsDir{ QDir(RuntimeInfo::dataPath()).filePath("gradients") }
    , m_defaultGradientName{ "_0012_Blue-Red_copy" }
{
    loadGradients();
}

GradientResourceManager::~GradientResourceManager() = default;

GradientResourceManager & GradientResourceManager::instance()
{
    static GradientResourceManager m;

    return m;
}

void GradientResourceManager::loadGradients()
{
    m_gradients.clear();

    // Gradient image visible in the UI
    const QSize gradientImageSize{ 200, 20 };
    // Input data used for the
    const QSize gradientLutSize{ 255, 1 };

    // navigate to the gradient directory
    QDir dir;
    bool dirNotFound = false;
    if (!dir.cd(m_gradientsDir))
    {
        dirNotFound = true;
    }
    else
    {
        dir.setFilter(QDir::Files | QDir::Readable | QDir::Hidden);

        for (const auto & fileInfo : dir.entryInfoList())
        {
            QPixmap pixmap;
            if (!pixmap.load(fileInfo.absoluteFilePath()))
            {
                qDebug() << "Unsupported file in gradient directory:" << fileInfo.fileName();
                continue;
            }

            const auto gradientImageScale = pixmap.scaled(gradientImageSize);
            const auto gradientLutInput = pixmap.width() <= gradientLutSize.width()
                ? pixmap : pixmap.scaled(gradientLutSize);

            m_gradients.emplace(fileInfo.baseName(),
                GradientData({ buildLookupTable(gradientLutInput), gradientImageScale }));
        }
    }

    // fall back, in case we didn't find any gradient images
    if (m_gradients.empty())
    {
        const auto fallbackMsg = "; only a fall-back gradient will be available\n\t(searching in " + m_gradientsDir + ")";
        if (dirNotFound)
        {
            qDebug() << "gradient directory does not exist" + fallbackMsg;
        }
        else
        {
            qDebug() << "gradient directory is empty" + fallbackMsg;
        }

        m_defaultGradientName = "fallback gradient";
        m_gradients.emplace(m_defaultGradientName, buildFallbackGradient(gradientImageSize));
    }
}

const QString & GradientResourceManager::gradientsDir() const
{
    return m_gradientsDir;
}

vtkSmartPointer<vtkLookupTable> GradientResourceManager::buildLookupTable(const QPixmap & pixmap)
{
    const auto image = pixmap.toImage();

    // use alpha = 1.0, if the image doesn't have a alpha channel
    int alphaMask = image.hasAlphaChannel() ? 0x00 : 0xFF;

    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(image.width());
    for (int i = 0; i < image.width(); ++i)
    {
        QRgb color = image.pixel(i, 0);
        lut->SetTableValue(i, qRed(color) / 255.0, qGreen(color) / 255.0, qBlue(color) / 255.0, (alphaMask | qAlpha(color)) / 255.0);
    }

    lut->SetNanColor(l_defaultNanColor);
    lut->BuildSpecialColors();

    return lut;
}

auto GradientResourceManager::buildFallbackGradient(const QSize & pixmapSize) -> GradientData
{
    auto gradientData = GradientData();

    gradientData.lookupTable = vtkSmartPointer<vtkLookupTable>::New();
    gradientData.lookupTable->SetNumberOfTableValues(pixmapSize.width());
    gradientData.lookupTable->Build();

    QImage image(pixmapSize, QImage::Format_RGBA8888);
    for (int i = 0; i < pixmapSize.width(); ++i)
    {
        double colorF[4];
        gradientData.lookupTable->GetTableValue(i, colorF);
        auto colorUI = vtkColorToQColor(colorF).rgba();
        for (int l = 0; l < pixmapSize.height(); ++l)
        {
            image.setPixel(i, l, colorUI);
        }
    }

    gradientData.lookupTable->SetNanColor(l_defaultNanColor);
    gradientData.lookupTable->BuildSpecialColors();

    gradientData.pixmap = QPixmap::fromImage(image);

    return gradientData;
}
