#include "GradientResourceManager.h"

#include <QDebug>
#include <QDir>
#include <QPixmap>

#include <vtkLookupTable.h>

#include <core/utility/qthelper.h>


auto GradientResourceManager::gradients() const -> const std::map<QString, const GradientData> &
{
    return m_gradients;
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
    : m_gradientDir{ "data/gradients" }
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

    const QSize gradientImageSize{ 200, 20 };

    // navigate to the gradient directory
    QDir dir;
    bool dirNotFound = false;
    if (!dir.cd(m_gradientDir))
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

            pixmap = pixmap.scaled(gradientImageSize);

            m_gradients.emplace(fileInfo.baseName(),
                GradientData({ buildLookupTable(pixmap), pixmap }));
        }
    }

    // fall back, in case we didn't find any gradient images
    if (m_gradients.empty())
    {
        const auto fallbackMsg = "; only a fall-back gradient will be available\n\t(searching in " + QDir().absoluteFilePath(m_gradientDir) + ")";
        if (dirNotFound)
        {
            qDebug() << "gradient directory does not exist" + fallbackMsg;
        }
        else
        {
            qDebug() << "gradient directory is empty" + fallbackMsg;
        }

        auto gradient = vtkSmartPointer<vtkLookupTable>::New();
        gradient->SetNumberOfTableValues(gradientImageSize.width());
        gradient->Build();

        QImage image(gradientImageSize, QImage::Format_RGBA8888);
        for (int i = 0; i < gradientImageSize.width(); ++i)
        {
            double colorF[4];
            gradient->GetTableValue(i, colorF);
            auto colorUI = vtkColorToQColor(colorF).rgba();
            for (int l = 0; l < gradientImageSize.height(); ++l)
                image.setPixel(i, l, colorUI);
        }

        m_gradients.emplace(QString(), GradientData{ gradient, QPixmap::fromImage(image) });
        m_defaultGradientName = QString();
    }
}

const QString & GradientResourceManager::gradientDir() const
{
    return m_gradientDir;
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

    // transparent NaN-color currently not correctly supported
    //lut->SetNanColor(1, 1, 1, 0);   // transparent!
    lut->SetNanColor(1, 1, 1, 1);

    return lut;
}