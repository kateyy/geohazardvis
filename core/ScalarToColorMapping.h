#pragma once

#include <QList>
#include <QMap>
#include <QString>


class QImage;

class RenderedData;
class ScalarsForColorMapping;


/**
Sets up scalar to surface color mapping for rendered data and stores the configuration state.
Uses ScalarsForColorMapping to determine scalars that can be mapped on the supplied renderedData.
*/
class ScalarToColorMapping
{
public:
    ScalarToColorMapping();

    void setRenderedData(const QList<RenderedData *> & renderedData);

    void clear();

    /** list of scalar names that can be used with my rendered data */
    QList<QString> scalarsNames() const;
    /** list of scalars that can be used with my rendered data */
    const QMap<QString, const ScalarsForColorMapping *> & scalars() const;

    QString currentScalars() const;
    void setCurrentScalars(QString scalarsName);

    /** @return currently used gradient image, in case it was previously set */
    const QImage * gradient() const;
    void setGradient(const QImage * gradientImage);

private:
    QList<RenderedData *> m_renderedData;

    QMap<QString, ScalarsForColorMapping *> m_scalars;

    QString m_currentScalars;
    const QImage * m_gradient;
};
