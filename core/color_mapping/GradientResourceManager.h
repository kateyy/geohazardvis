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

    const QString & gradientDir() const;

    struct GradientData
    {
        vtkSmartPointer<vtkLookupTable> lookupTable;
        QPixmap pixmap;
    };

    const std::map<QString, const GradientData> & gradients() const;

    const QString & defaultGradientName() const;
    void setDefaultGradientName(const QString & name);
    const GradientData & defaultGradient() const;

private:
    GradientResourceManager();
    ~GradientResourceManager();

    static vtkSmartPointer<vtkLookupTable> buildLookupTable(const QPixmap & pixmap);

private:
    const QString m_gradientDir;

    std::map<QString, const GradientData> m_gradients;
    QString m_defaultGradientName;
};
