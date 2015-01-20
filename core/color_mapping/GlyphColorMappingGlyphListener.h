#pragma once

#include <QList>
#include <QObject>


class AbstractVisualizedData;
class RenderedData3D;


class GlyphColorMappingGlyphListener : public QObject
{
    Q_OBJECT

public:
    GlyphColorMappingGlyphListener(QObject * parent = nullptr);

    void setData(const QList<AbstractVisualizedData *> & visualizedData);

signals:
    void glyphMappingChanged();

private:
    QList<RenderedData3D *> m_data;
    QList<QMetaObject::Connection> m_connects;
};
