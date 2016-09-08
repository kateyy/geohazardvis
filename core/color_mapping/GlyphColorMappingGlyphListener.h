#pragma once

#include <QList>
#include <QObject>


class AbstractVisualizedData;
class RenderedData3D;


class GlyphColorMappingGlyphListener : public QObject
{
    Q_OBJECT

public:
    explicit GlyphColorMappingGlyphListener(QObject * parent = nullptr);
    ~GlyphColorMappingGlyphListener() override;

    void setData(const QList<AbstractVisualizedData *> & visualizedData);

signals:
    void glyphMappingChanged();

private:
    QList<RenderedData3D *> m_data;
    QList<QMetaObject::Connection> m_connects;

private:
    Q_DISABLE_COPY(GlyphColorMappingGlyphListener)
};
